#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <png.h>
#include "uselibpng.h"

using namespace std;

struct Vertex {
    float x, y, z, w;
    float r, g, b;
};

vector<Vertex> vertices;
vector<int> indices;
int width, height;
string outputFilename;

// Parse input file function remains the same
void parseInputFile(const string& filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;  // Skip empty lines and comments
        
        istringstream iss(line);
        string keyword;
        iss >> keyword;
        if (keyword == "png") {
            iss >> width >> height >> outputFilename;
        } else if (keyword == "position") {
            int count;
            iss >> count;
            for (int i = 0; i < count; ++i) {
                Vertex v;
                iss >> v.x >> v.y >> v.z >> v.w;
                vertices.push_back(v);
            }
        } else if (keyword == "color") {
            int count;
            iss >> count;
            for (int i = 0; i < count; ++i) {
                float r, g, b;
                iss >> r >> g >> b;
                if (!vertices.empty()) {
                    vertices[i].r = r;
                    vertices[i].g = g;
                    vertices[i].b = b;
                }
            }
        } else if (keyword == "drawArraysTriangles") {
            int start, count;
            iss >> start >> count;
            for (int i = 0; i < count; ++i) {
                indices.push_back(start + i);
            }
        }
    }
}

// Interpolate values based on barycentric coordinates
float interpolate(float v0, float v1, float v2, float w0, float w1, float w2) {
    return v0 * w0 + v1 * w1 + v2 * w2;
}

// Calculate barycentric coordinates
void computeBarycentricCoordinates(float x, float y, 
                                 const Vertex& v0, const Vertex& v1, const Vertex& v2,
                                 float& w0, float& w1, float& w2) {
    float denominator = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
    w0 = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / denominator;
    w1 = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / denominator;
    w2 = 1.0f - w0 - w1;
}

void rasterizeTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, 
                      vector<vector<uint8_t>>& imageR,
                      vector<vector<uint8_t>>& imageG,
                      vector<vector<uint8_t>>& imageB) {
    // Find bounding box
    int minX = max(0, (int)floor(min({v0.x, v1.x, v2.x})));
    int maxX = min(width - 1, (int)ceil(max({v0.x, v1.x, v2.x})));
    int minY = max(0, (int)floor(min({v0.y, v1.y, v2.y})));
    int maxY = min(height - 1, (int)ceil(max({v0.y, v1.y, v2.y})));

    // Rasterize
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float w0, w1, w2;
            computeBarycentricCoordinates(x + 0.5f, y + 0.5f, v0, v1, v2, w0, w1, w2);
            
            // Check if point is inside triangle
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                // Interpolate colors
                float r = interpolate(v0.r, v1.r, v2.r, w0, w1, w2);
                float g = interpolate(v0.g, v1.g, v2.g, w0, w1, w2);
                float b = interpolate(v0.b, v1.b, v2.b, w0, w1, w2);
                
                // Convert to 8-bit color
                imageR[y][x] = (uint8_t)(r * 255);
                imageG[y][x] = (uint8_t)(g * 255);
                imageB[y][x] = (uint8_t)(b * 255);
            }
        }
    }
}

void scanlineAlgorithm(vector<vector<uint8_t>>& imageR,
                      vector<vector<uint8_t>>& imageG,
                      vector<vector<uint8_t>>& imageB) {
    // Process each triangle
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (indices[i] < vertices.size() && indices[i + 1] < vertices.size() && indices[i + 2] < vertices.size()) {
            Vertex v0 = vertices[indices[i]];
            Vertex v1 = vertices[indices[i + 1]];
            Vertex v2 = vertices[indices[i + 2]];

            // Apply viewport transformation
            v0.x = (v0.x / v0.w + 1) * width / 2;
            v0.y = (v0.y / v0.w + 1) * height / 2;
            v1.x = (v1.x / v1.w + 1) * width / 2;
            v1.y = (v1.y / v1.w + 1) * height / 2;
            v2.x = (v2.x / v2.w + 1) * width / 2;
            v2.y = (v2.y / v2.w + 1) * height / 2;

            rasterizeTriangle(v0, v1, v2, imageR, imageG, imageB);
        }
    }
}

void savePNG(const string& filename, 
             const vector<vector<uint8_t>>& imageR,
             const vector<vector<uint8_t>>& imageG,
             const vector<vector<uint8_t>>& imageB) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    vector<uint8_t> row(width * 3);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            row[x * 3 + 0] = imageR[y][x];
            row[x * 3 + 1] = imageG[y][x];
            row[x * 3 + 2] = imageB[y][x];
        }
        png_write_row(png, row.data());
    }

    png_write_end(png, NULL);
    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input file>" << endl;
        return 1;
    }

    parseInputFile(argv[1]);

    // Initialize separate color channels
    vector<vector<uint8_t>> imageR(height, vector<uint8_t>(width, 0));
    vector<vector<uint8_t>> imageG(height, vector<uint8_t>(width, 0));
    vector<vector<uint8_t>> imageB(height, vector<uint8_t>(width, 0));

    scanlineAlgorithm(imageR, imageG, imageB);
    savePNG(outputFilename, imageR, imageG, imageB);

    return 0;
}