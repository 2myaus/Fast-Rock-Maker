#include <stdint.h>
#include <random>
#include <fstream>
#include <string.h>
#include <functional>
#include <array>
#include <thread>
#include <mutex>

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
            const unsigned short numThreads = 16;

            const unsigned int waterPoints = waterDensity * width * height;

            uint8_t *waterfied = (uint8_t*)malloc(sizeof(bool) * width * height); //First bit = filled, second = on the filling surface
            std::fill(waterfied, waterfied + width * height, 0);

            struct waterSurfaceNode
            {
                waterSurfaceNode *left;
                waterSurfaceNode *right;
                unsigned int x;
                unsigned int y;
                float density;
            };

            waterSurfaceNode *nodeArray = (waterSurfaceNode*)malloc(sizeof(waterSurfaceNode) * width * height);
            unsigned long numNodes = 0;

            waterSurfaceNode *headNode = nullptr;

            std::mutex nodeLock;
            const std::function<void(unsigned int, unsigned int)> addSurfaceNode = [this, &headNode, &waterfied, &nodeLock, &nodeArray, &numNodes](unsigned int x, unsigned int y) -> void{
                unsigned int gridoff = y * width + x;
                std::lock_guard<std::mutex> lock(nodeLock);
                if(waterfied[gridoff] >> 1) return;
                waterfied[gridoff] |= 0b00000010;
                waterSurfaceNode *current = headNode;
                float density = getBlock(x, y)->density;
                if(current == nullptr){
                    headNode = nodeArray + numNodes;
                    numNodes++;
                    current = headNode;
                }
                else{
                    while (true) {
                        if (density <= current->density) {
                            if (current->left == nullptr) {
                                current->left = nodeArray + numNodes;
                                numNodes++;
                                current = current->left;
                                break;
                            }
                            current = current->left;
                        }
                        else if (density > current->density) {
                            if (current->right == nullptr) {
                                current->right = nodeArray + numNodes;
                                numNodes++;
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

            const std::function<void()> removeWeakestNode = [&headNode, &nodeLock]() -> void{
                std::lock_guard<std::mutex> lock(nodeLock);
                waterSurfaceNode *current = headNode;
                waterSurfaceNode *parent = nullptr;
                while(current->left != nullptr){
                    parent = current;
                    current = current->left;
                }
                if(parent == nullptr){
                    headNode = current->right;
                    //free(current);
                    return;
                }
                parent->left = current->right;
                //free(current);
            };

            const std::function<waterSurfaceNode()> getAndRemoveWeakestNode = [&headNode, &nodeLock]() -> waterSurfaceNode{
                std::lock_guard<std::mutex> lock(nodeLock);
                waterSurfaceNode *current = headNode;
                waterSurfaceNode *parent = nullptr;
                while(current->left != nullptr){
                    parent = current;
                    current = current->left;
                }
                if(parent == nullptr){
                    headNode = current->right;
                    goto freeAndReturn;
                }
                parent->left = current->right;

freeAndReturn:
                waterSurfaceNode copy = *current;
                return copy;
            };

            addSurfaceNode(width / 2, 0);

            unsigned int i = 0;
            const std::function<void()> erodeTunnels = [&i, &waterPoints, &addSurfaceNode, &getAndRemoveWeakestNode, &waterfied, this]() -> void{
                while(i < waterPoints){
                    const waterSurfaceNode weakestNode = getAndRemoveWeakestNode();

                    if(waterfied[weakestNode.y * width + weakestNode.x] & 0b00000001){
                        throw "Already waterfied!";
                    }
                    emptyBlock(weakestNode.x, weakestNode.y);
                    waterfied[weakestNode.y * width + weakestNode.x] |= 0b00000001;

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
                    i++;
                }
            };

            std::thread threads[numThreads];
            for(unsigned short n = 0; n < numThreads; n++){
                threads[n] = std::thread(erodeTunnels);
            }
            for(unsigned short n = 0; n < numThreads; n++){
                threads[n].join();
            }

            /*
            while(headNode != nullptr){
                removeWeakestNode();
            }
            */
           free(waterfied);
           free(nodeArray);
        }

        void SmoothCellSection(unsigned int xi, unsigned int yi, unsigned int xf, unsigned int yf){ //Smooth a rectangular area from xi, yi to xf, yf excluding xf/yf
            for(unsigned int x = xi; x < xf; x++){
                for(unsigned int y = yi; y < yf; y++){
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

        void ThreadedCellSmooth(unsigned short horSecs, unsigned short vertSecs){ //Number of threads = horSecs * vertSecs

            unsigned int horDivisions[horSecs + 1];
            unsigned int vertDivisions[vertSecs + 1];

            for(unsigned short horSec = 0; horSec < horSecs; horSec++){
                horDivisions[horSec] = width * horSec / horSecs;
            }
            for(unsigned short vertSec = 0; vertSec < vertSecs; vertSec++){
                vertDivisions[vertSec] = height * vertSec / vertSecs;
            }
            horDivisions[horSecs] = width;
            vertDivisions[vertSecs] = height;

            std::thread threads[horSecs * vertSecs];

            for(unsigned short horSec = 0; horSec < horSecs; horSec++){
                for(unsigned short vertSec = 0; vertSec < vertSecs; vertSec++){
                    unsigned int xi = horDivisions[horSec];
                    unsigned int xf = horDivisions[horSec + 1];
                    unsigned int yi = vertDivisions[vertSec];
                    unsigned int yf = vertDivisions[vertSec + 1];
                    threads[vertSec * horSecs + horSec] = std::thread(&fastrm::Cave::SmoothCellSection, this, xi, yi, xf, yf);
                }
            }

            for(unsigned short t = 0; t < horSecs * vertSecs; t++){
                threads[t].join();
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
            ThreadedCellSmooth(5, 5);
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
