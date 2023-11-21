#include "../fastrm.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

int main(){
    fastrm::Cave cave = fastrm::Cave(2000, 2000);

    long start = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()
    ).count();
    cave.PopulateDensity();
    printf("Population done\n");
    cave.Erode();
    printf("Erosion done\n");
    cave.Smooth();
    printf("Smoothing done\n");
    long end = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()
    ).count();

    printf("Done! Took %ldms\n", end - start);

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
