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

void rasterizeTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Image& img) {
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
                
                // Convert to 8-bit color and set pixel
                img[y][x].red = (uint8_t)(r * 255);
                img[y][x].green = (uint8_t)(g * 255);
                img[y][x].blue = (uint8_t)(b * 255);
                img[y][x].alpha = 0xFF;  // Fully opaque where triangles are drawn
            }
        }
    }
}

void scanlineAlgorithm(Image& img) {
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

            rasterizeTriangle(v0, v1, v2, img);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input file>" << endl;
        return 1;
    }

    parseInputFile(argv[1]);

    // Create image with transparent background
    Image img(width, height);
    
    // Initialize the image with transparent background
    for (int y = 0; y < img.height(); y++) {
        for (int x = 0; x < img.width(); x++) {
            img[y][x].red = 0;
            img[y][x].green = 0;
            img[y][x].blue = 0;
            img[y][x].alpha = 0;  // Set alpha to 0 for full transparency
        }
    }

    // Debug print indices
    cout << "Indices: ";
    for (int idx : indices) {
        cout << idx << " ";
    }
    cout << endl;

    // Print vertices with color information
    cout << "Vertices: " << endl;
    for (const Vertex& v : vertices) {
        cout << v.x << " " << v.y << " " << v.z << " " << v.w << " " 
             << v.r << " " << v.g << " " << v.b << endl;
    }

    scanlineAlgorithm(img);
    img.save(outputFilename.c_str());

    return 0;
}