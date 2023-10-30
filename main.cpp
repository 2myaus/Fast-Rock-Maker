#include <stdint.h>
#include <random>
#include <fstream>
#include <string.h>
#include <functional>
#include <array>

namespace fastrm{
    class Block{
        private:

        public:

        float density;

        Block(){
            this->density = 0;
        }
    };
    class Cave{
        private:

        Block *grid;
        unsigned int width;
        unsigned int height;

        void AllocateGrid(unsigned int width, unsigned int height){
            this->grid = new Block[width * height];
            this->width = width;
            this->height = height;

            for(unsigned long n = 0; n < width * height; n++){
                grid[n] = Block();
            }
        }

        void emptyBlock(unsigned int x, unsigned int y){
            grid[y * width + x].density = 0;
        }

        void PopulateCrockNoise(){
            const double pointDensity = 0.02;
            const unsigned int meanRadius = 10;
            const unsigned int maxRadiusDeviation = 5;

            const unsigned int numPoints = pointDensity * width * height;

            std::random_device rd;
            std::mt19937 gen(rd());

            std::uniform_int_distribution<unsigned int> randR(meanRadius - maxRadiusDeviation, meanRadius + maxRadiusDeviation);

            for(unsigned int i = 0; i < numPoints; i++){
                const unsigned int r = randR(gen);

                std::uniform_int_distribution<unsigned int> randX(0, width - 1 + 2 * r);
                std::uniform_int_distribution<unsigned int> randY(0, height - 1 + 2 * r);

                const unsigned int cx = randX(gen);
                const unsigned int cy = randY(gen);

                const unsigned int r2 = r * r;

                for(unsigned int dx = 0; dx <= r; dx++){
                    for(unsigned int dy = 0; dy <= r; dy++){
                        const unsigned int mag2 = dx * dx + dy * dy;
                        if(mag2 <= r2){
                            bool lXInBounds = cx >= dx + r && cx - dx - r < width;
                            bool rXInBounds = cx + dx >= + r && cx + dx - r < width;

                            bool uYInBounds = cy >= dy + r && cy - dy - r < height;
                            bool lYInBounds = cy + dy >= + r && cy + dy - r < height;

                            /*
                            if(lXInBounds && uYInBounds) getBlock(cx - dx - r, cy - dy - r)->density += 1.0 - static_cast<float>(mag2) / r2;
                            if(lXInBounds && lYInBounds) getBlock(cx - dx - r, cy + dy - r)->density += 1.0 - static_cast<float>(mag2) / r2;
                            if(rXInBounds && uYInBounds) getBlock(cx + dx - r, cy - dy - r)->density += 1.0 - static_cast<float>(mag2) / r2;
                            if(rXInBounds && lYInBounds) getBlock(cx + dx - r, cy + dy - r)->density += 1.0 - static_cast<float>(mag2) / r2;
                            */

                            const double mag = sqrt(mag2);
                            const double densitychange = 1.0 - mag / r;
                            if(lXInBounds && uYInBounds) getBlock(cx - dx - r, cy - dy - r)->density += densitychange;
                            if(dy > 0){
                                if(lXInBounds && lYInBounds) getBlock(cx - dx - r, cy + dy - r)->density += densitychange;
                            }
                            if(dx > 0){
                                if(rXInBounds && uYInBounds) getBlock(cx + dx - r, cy - dy - r)->density += densitychange;
                                if(dy > 0){
                                    if(rXInBounds && lYInBounds) getBlock(cx + dx - r, cy + dy - r)->density += densitychange;
                                }
                            }
                        }
                    }
                }
            }
        }

        void ErodeCrockTunnels(){
            const float waterDensity = 0.6;

            const unsigned int waterPoints = waterDensity * width * height;

            bool *waterfied = (bool*)malloc(sizeof(bool) * width * height);
            bool *waterSurface = (bool*)malloc(sizeof(bool) * width * height);
            //bool waterfied[width * height];
            //bool waterSurface[width * height];
            std::fill(waterfied, waterfied + width * height, false);
            std::fill(waterSurface, waterSurface + width * height, false);

            struct waterSurfaceNode
            {
                waterSurfaceNode *left;
                waterSurfaceNode *right;
                unsigned int x;
                unsigned int y;
                float density;
            };

            waterSurfaceNode *headNode = nullptr;

            const std::function<void(unsigned int x, unsigned int y)> addSurfaceNode = [this, &headNode, &waterSurface](unsigned int x, unsigned int y) -> void{
                unsigned int gridoff = y * width + x;
                if(waterSurface[gridoff]) return;
                waterSurface[gridoff] = true;
                waterSurfaceNode *current = headNode;
                float density = getBlock(x, y)->density;
                if(current == nullptr && headNode == nullptr){
                    headNode = (waterSurfaceNode *)malloc(sizeof(waterSurfaceNode));
                    current = headNode;
                }
                else{
                    while (true) {
                    if (density < current->density) {
                        if (current->left == nullptr) {
                            current->left = (waterSurfaceNode *)malloc(sizeof(waterSurfaceNode));
                            current = current->left;
                            break;
                        }
                        current = current->left;
                    } else if (density >= current->density) {
                        if (current->right == nullptr) {
                            current->right = (waterSurfaceNode *)malloc(sizeof(waterSurfaceNode));
                            current = current->right;
                            break;
                        }
                        current = current->right;
                    }
                }
                }

                current->left = nullptr;
                current->right = nullptr;
                current->x = x;
                current->y = y;
                current->density = density;
            };

            const std::function<waterSurfaceNode*()> getWeakestNode = [&headNode]() -> waterSurfaceNode*{
                waterSurfaceNode *current = headNode;
                while(current->left != nullptr){
                    current = current->left;
                }
                return current;
            };

            const std::function<void()> removeWeakestNode = [&headNode]() -> void{
                waterSurfaceNode *current = headNode;
                waterSurfaceNode *parent = nullptr;
                while(current->left != nullptr){
                    parent = current;
                    current = current->left;
                }
                if(parent == nullptr){
                    headNode = current->right;
                    free(current);
                    return;
                }
                parent->left = current->right;
                free(current);
            };

            const std::function<void(const waterSurfaceNode*)> freeNode = [&freeNode, &headNode, &removeWeakestNode](const waterSurfaceNode *toFree) -> void{
                // if(toFree->left != nullptr){
                //     freeNode(toFree->left);
                // }
                // if(toFree->right != nullptr){
                //     freeNode(toFree->left);
                // }
                // free((void*)toFree);
                while(headNode != nullptr){
                    removeWeakestNode();
                }
            };

            addSurfaceNode(width / 2, 0);

            for(unsigned int i = 0; i < waterPoints; i++){
                const waterSurfaceNode weakestNode = *getWeakestNode();
                //TODO: Add getAndRemove
                removeWeakestNode();

                if(waterfied[weakestNode.y * width + weakestNode.x]){
                    throw "Already waterfied!";
                }
                emptyBlock(weakestNode.x, weakestNode.y);
                waterfied[weakestNode.y * width + weakestNode.x] = true;

                if(weakestNode.x > 0){
                    addSurfaceNode(weakestNode.x - 1, weakestNode.y);
                }
                if(weakestNode.x < this->width - 1){
                    addSurfaceNode(weakestNode.x + 1, weakestNode.y);
                }

                if(weakestNode.y > 0){
                    addSurfaceNode(weakestNode.x, weakestNode.y - 1);
                }
                if(weakestNode.y < this->height - 1){
                    addSurfaceNode(weakestNode.x, weakestNode.y + 1);
                }
            }

            freeNode(headNode);
        }

        void CellSmooth(){
            for(unsigned int x = 0; x < width; x++){
                for(unsigned int y = 0; y < width; y++){
                    unsigned short numNeighbors = 0;
                    
                    if(x>0 && getBlock(x-1,y)->density > 0) numNeighbors++;
                    if(x+1<width && getBlock(x+1,y)->density > 0) numNeighbors++;
                    if(y>0 && getBlock(x,y-1)->density > 0) numNeighbors++;
                    if(y+1<height && getBlock(x,y+1)->density > 0) numNeighbors++;
                    if(x>0 && y>0 && getBlock(x-1,y-1)->density > 0) numNeighbors++;
                    if(x+1<width && y>0 && getBlock(x+1,y-1)->density > 0) numNeighbors++;
                    if(x>0 && y+1<height && getBlock(x-1,y+1)->density > 0) numNeighbors++;
                    if(x+1<width && y+1<height && getBlock(x+1,y+1)->density > 0) numNeighbors++;

                    if(numNeighbors < 4){
                        emptyBlock(x,y);
                    }
                }
            }
        }

        public:

        unsigned int getWidth(){
            return width;
        }
        unsigned int getHeight(){
            return height;
        }

        Block *getBlock(unsigned int x, unsigned int y){
            return &(this->grid[y * width + x]);
        }

        void setBlock(unsigned int x, unsigned int y, Block block){
            this->grid[y * width + x] = block;
        }

        Cave(unsigned int width, unsigned int height){
            AllocateGrid(width, height);
        }
        ~Cave(){
            delete[] grid;
        }

        void PopulateDensity(){
            PopulateCrockNoise();
        }
        void Erode(){
            ErodeCrockTunnels();
        }
        void Smooth(){
            CellSmooth();
        }
    };
}

int main(){
    fastrm::Cave cave = fastrm::Cave(2000, 2000);
    cave.PopulateDensity();
    printf("Population done\n");
    cave.Erode();
    printf("Erosion done\n");
    cave.Smooth();
    printf("Smoothing done\n");

    float maxVal = 0;
    for (unsigned int y = 0; y < cave.getHeight(); y++) {
        for (unsigned int x = 0; x < cave.getWidth(); x++) {
            maxVal = std::max(maxVal, cave.getBlock(x, y)->density);
        }
    }

    // Create a vector to store the image data (grayscale).

    // Write the image data to a PNG file.
    std::ofstream image("heatmap.ppm");
    image << "P3" << std::endl;
    image << cave.getWidth() << " " << cave.getHeight() << std::endl;
    image << "255" << std::endl; // Max color value

    // Loop through the heatmap data and fill the image buffer.
    for (unsigned int y = 0; y < cave.getHeight(); y++) {
        for (unsigned int x = 0; x < cave.getWidth(); x++) {
            uint8_t intensity = static_cast<uint8_t>(255 * cave.getBlock(x, y)->density / maxVal);
            if(intensity == 0){
                image << "255 0 0 ";
            }
            else{
                for(int i = 0; i < 3; i++){
                    image << std::to_string(intensity) << ' ';
                }
            }
        }
        image << '\n';
    }

    image << std::endl;

    return 0;
}
