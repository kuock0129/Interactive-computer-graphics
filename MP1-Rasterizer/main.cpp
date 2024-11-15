#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <limits>
#include <png.h>
#include "uselibpng.h"

using namespace std;

struct Position {
    float x, y, z, w;
    Position() : x(0), y(0), z(0), w(1) {}
};

struct Color {
    float r, g, b, a;
    Color() : r(0), g(0), b(0), a(1) {}
};

struct Vertex {
    float x, y, z, w;
    float r, g, b, a;
};

// Separate vectors for positions and colors
vector<Position> positions;
vector<Color> colors;
vector<Vertex> vertices;  // Combined vertices after parsing
vector<vector<float>> depthBuffer;
vector<int> elements;
bool depthTestEnabled = false;

int width, height;
string outputFilename;

// Function prototypes
void combineVertices();
void parseInputFile(const string& filename, Image& img);
void scanlineAlgorithm(Image& img, const vector<int>& currentIndices);
void rasterizeTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Image& img);
float interpolate(float v0, float v1, float v2, float w0, float w1, float w2);
void computeBarycentricCoordinates(float x, float y, const Vertex& v0, const Vertex& v1, const Vertex& v2, float& w0, float& w1, float& w2);

void combineVertices() {
    vertices.clear();  // Clear previous vertices
    size_t vertexCount = max(positions.size(), colors.size());
    vertices.resize(vertexCount);
    
    // Default color (white) for vertices without color
    Color defaultColor;
    defaultColor.r = 1.0f;
    defaultColor.g = 1.0f;
    defaultColor.b = 1.0f;
    defaultColor.a = 1.0f;
    
    // Default position (origin) for colors without position
    Position defaultPosition;
    
    for (size_t i = 0; i < vertexCount; ++i) {
        Vertex& v = vertices[i];
        
        // Get position (use default if not enough positions)
        const Position& pos = (i < positions.size()) ? positions[i] : defaultPosition;
        v.x = pos.x;
        v.y = pos.y;
        v.z = pos.z;
        v.w = pos.w;
        
        // Get color (use default if not enough colors)
        const Color& col = (i < colors.size()) ? colors[i] : defaultColor;
        v.r = col.r;
        v.g = col.g;
        v.b = col.b;
        v.a = col.a;
    }
}

void parseInputFile(const string& filename, Image& img) {
    ifstream file(filename);
    string line;
    
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        istringstream iss(line);
        string keyword;
        iss >> keyword;

        if (keyword == "depth") {
            depthTestEnabled = true;
            depthBuffer.resize(height, vector<float>(width, numeric_limits<float>::infinity()));
        }
        else if (keyword == "color") {
            colors.clear();  // Clear existing colors
            int size;
            iss >> size;
            
            while (true) {
                Color color;
                if (!(iss >> color.r >> color.g >> color.b)) break;
                if (size == 4 && !(iss >> color.a)) break;
                colors.push_back(color);
            }
            cout << "Parsed " << colors.size() << " colors" << endl;
        }
        else if (keyword == "position") {
            positions.clear();  // Clear existing positions
            int size;
            iss >> size;
            
            while (true) {
                Position pos;
                if (!(iss >> pos.x >> pos.y)) break;
                pos.z = 0.0f;
                pos.w = 1.0f;
                if (size >= 3 && !(iss >> pos.z)) break;
                if (size == 4 && !(iss >> pos.w)) break;
                positions.push_back(pos);
            }
            cout << "Parsed " << positions.size() << " positions" << endl;
        }
        else if (keyword == "elements") {
            elements.clear();  // Clear existing elements
            int index;
            while (iss >> index) {
                elements.push_back(index);
            }
            cout << "Parsed " << elements.size() << " elements" << endl;
            // for (int i = 0; i < elements.size(); i++) {
            //     cout << elements[i] << " ";
            // }
            cout << endl;
        }
        else if (keyword == "drawArraysTriangles") {
            int start, count;
            iss >> start >> count;
            
            // Combine vertices before creating indices
            combineVertices();
            
            // Create indices for this draw call
            vector<int> currentIndices;
            for (int i = 0; i < count; ++i) {
                currentIndices.push_back(start + i);
            }
            
            // Process this set of triangles
            scanlineAlgorithm(img, currentIndices);
        }
        else if (keyword == "drawElementsTriangles") {
            int count, offset;
            iss >> count >> offset;

            // Combine vertices before creating indices
            combineVertices();
            
            // Create indices for this draw call
            vector<int> currentIndices;
            for (int i = 0; i < count; ++i) {
                currentIndices.push_back(elements[offset + i]);
            }
            
            // Process this set of triangles
            scanlineAlgorithm(img, currentIndices);
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
                float z = interpolate(v0.z, v1.z, v2.z, w0, w1, w2);

                if (!depthTestEnabled || z < depthBuffer[y][x]) {
                    // Interpolate colors
                    float r = interpolate(v0.r, v1.r, v2.r, w0, w1, w2);
                    float g = interpolate(v0.g, v1.g, v2.g, w0, w1, w2);
                    float b = interpolate(v0.b, v1.b, v2.b, w0, w1, w2);

                    // Check bounds before accessing img
                    if (y >= 0 && y < height && x >= 0 && x < width) {
                        // Convert to 8-bit color and set pixel
                        img[y][x].red = (uint8_t)(r * 255);
                        img[y][x].green = (uint8_t)(g * 255);
                        img[y][x].blue = (uint8_t)(b * 255);
                        img[y][x].alpha = 0xFF;  // Fully opaque where triangles are drawn
                        if (depthTestEnabled) {
                            depthBuffer[y][x] = z;
                        }
                    } else {
                        cout << "Pixel out of bounds: (" << x << ", " << y << ")" << endl;
                    }
                }
            }
        }
    }
}



void scanlineAlgorithm(Image& img, const vector<int>& currentIndices) {

    // for(int i = 0; i < currentIndices.size(); i++) {
    //     cout << currentIndices[i] << " ";
    // }
    // cout<<endl;
    // Process each triangle
    for (size_t i = 0; i < currentIndices.size(); i += 3) {
        if (i + 2 >= currentIndices.size()) break;
        
        size_t idx0 = currentIndices[i];
        size_t idx1 = currentIndices[i + 1];
        size_t idx2 = currentIndices[i + 2];

        cout << idx0 << " " << idx1 << " " << idx2 << endl;
        cout << vertices.size() << endl;
        
        if (idx0 < vertices.size() && idx1 < vertices.size() && idx2 < vertices.size()) {
            Vertex v0 = vertices[idx0];
            Vertex v1 = vertices[idx1];
            Vertex v2 = vertices[idx2];

            // Apply viewport transformation
            v0.x = (v0.x / v0.w + 1) * width / 2;
            v0.y = (v0.y / v0.w + 1) * height / 2;
            v1.x = (v1.x / v1.w + 1) * width / 2;
            v1.y = (v1.y / v1.w + 1) * height / 2;
            v2.x = (v2.x / v2.w + 1) * width / 2;
            v2.y = (v2.y / v2.w + 1) * height / 2;

            cout << "Triangle vertices after transform:" << endl;
            cout << v0.x << "," << v0.y << " " << v0.r << "," << v0.g << "," << v0.b << endl;
            cout << v1.x << "," << v1.y << " " << v1.r << "," << v1.g << "," << v1.b << endl;
            cout << v2.x << "," << v2.y << " " << v2.r << "," << v2.g << "," << v2.b << endl;

            rasterizeTriangle(v0, v1, v2, img);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input file>" << endl;
        return 1;
    }

    // First pass to get PNG parameters
    ifstream file(argv[1]);
    string line;
    bool foundPNG = false;
    
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        istringstream iss(line);
        string keyword;
        iss >> keyword;
        
        if (keyword == "png") {
            iss >> width >> height >> outputFilename;
            foundPNG = true;
            break;
        }
    }
    
    if (!foundPNG) {
        cerr << "Error: No PNG parameters found in input file" << endl;
        return 1;
    }

    // Create image and parse file
    Image img(width, height);
    cout << "Creating image: " << width << " x " << height << " -> " << outputFilename << endl;
    
    parseInputFile(argv[1], img);
    img.save(outputFilename.c_str());

    return 0;
}