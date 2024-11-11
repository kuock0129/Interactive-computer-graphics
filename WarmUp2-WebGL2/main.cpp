#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "uselibpng.h"

int main(int argc, char **argv) {
    std::ifstream infile(argv[1]);
    std::string line;

    int width, height;
    std::string filename;
    std::getline(infile, line);
    std::istringstream iss(line);
    std::string png;
    iss >> png >> width >> height >> filename;

    Image img = Image(width, height);
    // for (int y = 0; y < img.height(); y += 1) {
    //     for (int x = 0; x < img.width(); x += 1) {
    //         img[y][x].red = 192;
    //         img[y][x].green = 192;
    //         img[y][x].blue = 192;
    //         img[y][x].alpha = 0XFF;
    //     }
    // }

    std::vector<std::vector<int>> positions;
    std::vector<std::vector<int>> colors;
    int pixels = 0;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "position") {
            std::vector<std::vector<int>>().swap(positions);
            int elements;
            iss >> elements;

            std::vector<int> pos;
            int val;
            while (iss >> val) {
                pos.push_back(val);
                if (pos.size() == elements) {
                    positions.push_back(std::move(pos));
                }
            }
        } else if (keyword == "color") {
            std::vector<std::vector<int>>().swap(colors);
            int elements;
            iss >> elements;

            std::vector<int> color;
            int val;
            while (iss >> val) {
                color.push_back(val);
                if (color.size() == elements) {
                    colors.push_back(std::move(color));
                }
            }
        } else if (keyword == "drawPixels") {
            iss >> pixels;
            for (int i = 0; i < pixels; i++) {
                int x = positions[i][0];
                int y = positions[i][1];
                img[y][x].red = colors[i][0];
                img[y][x].green = colors[i][1];
                img[y][x].blue = colors[i][2];
                img[y][x].alpha = colors[i][3];
            }
        }
    }

    img.save(filename.c_str());

    infile.close();
}
