#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <memory>
#include <png.h>
#include <iostream>
#include <functional>
#include "uselibpng.h"

using namespace std;

// Global variables for image dimensions and output
int width, height;
string outputFilename;

// Structs instead of classes
struct Vertex {
    double x{0}, y{0}, z{0}, w{1.0}; // Position
    double r{0}, g{0}, b{0}, a{1.0}; // Color
    double s{0}, t{0};               // Texture coordinates
};

struct Pixel {
    uint8_t r, g, b, a;
};

struct Buffer {
    vector<Pixel> pixels;
    vector<double> zbuffer;
    uint32_t width;
    uint32_t height;
    bool useDepthTest{false};
    bool usesRGB{false};
};

// Vertex operations
Vertex transformVertex(const Vertex& v, const vector<double>& matrix) {
    Vertex result = v;
    vector<double> transformed(4, 0.0);
    const double values[4] = {v.x, v.y, v.z, v.w};
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            transformed[i] += matrix[i * 4 + j] * values[j];
        }
    }
    
    result.x = transformed[0];
    result.y = transformed[1];
    result.z = transformed[2];
    result.w = transformed[3];
    return result;
}

Vertex normalizeVertex(const Vertex& v, int width, int height, bool enableHyp) {
    Vertex result = v;
    if (result.w != 0) {
        result.x /= result.w;
        result.y /= result.w;
        result.z /= result.w;
        if (enableHyp) {
            result.r /= result.w;
            result.g /= result.w;
            result.b /= result.w;
            result.a /= result.w;
            result.s /= result.w;
            result.t /= result.w;
        }
    }
    result.x = (result.x + 1) * width / 2;
    result.y = (result.y + 1) * height / 2;
    return result;
}

Vertex vertexSubtract(const Vertex& a, const Vertex& b) {
    Vertex result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    result.r = a.r - b.r;
    result.g = a.g - b.g;
    result.b = a.b - b.b;
    result.a = a.a - b.a;
    result.s = a.s - b.s;
    result.t = a.t - b.t;
    return result;
}

Vertex vertexAdd(const Vertex& a, const Vertex& b) {
    Vertex result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    result.r = a.r + b.r;
    result.g = a.g + b.g;
    result.b = a.b + b.b;
    result.a = a.a + b.a;
    result.s = a.s + b.s;
    result.t = a.t + b.t;
    return result;
}

Vertex vertexScale(const Vertex& v, double scale) {
    Vertex result;
    result.x = v.x * scale;
    result.y = v.y * scale;
    result.z = v.z * scale;
    result.w = v.w * scale;
    result.r = v.r * scale;
    result.g = v.g * scale;
    result.b = v.b * scale;
    result.a = v.a * scale;
    result.s = v.s * scale;
    result.t = v.t * scale;
    return result;
}

// Buffer operations
Buffer createBuffer(uint32_t width, uint32_t height) {
    Buffer buffer;
    buffer.width = width;
    buffer.height = height;
    buffer.pixels.resize(width * height);
    buffer.zbuffer.resize(width * height, INFINITY);
    return buffer;
}

void setPixel(Buffer& buffer, const Vertex& v) {
    int x = static_cast<int>(v.x);
    int y = static_cast<int>(v.y);
    
    if (x < 0 || x >= static_cast<int>(buffer.width) || 
        y < 0 || y >= static_cast<int>(buffer.height)) {
        return;
    }

    size_t index = y * buffer.width + x;
    if (buffer.useDepthTest && buffer.zbuffer[index] <= v.z) {
        return;
    }

    double r = v.r, g = v.g, b = v.b, a = v.a;
    
    if (a < 1.0) {
        Pixel& existing = buffer.pixels[index];
        double existingA = existing.a / 255.0;
        double blendedA = a + existingA * (1 - a);
        
        if (blendedA > 0) {
            r = (r * a + (existing.r / 255.0) * existingA * (1 - a)) / blendedA;
            g = (g * a + (existing.g / 255.0) * existingA * (1 - a)) / blendedA;
            b = (b * a + (existing.b / 255.0) * existingA * (1 - a)) / blendedA;
            a = blendedA;
        }
    }

    buffer.pixels[index].r = static_cast<uint8_t>(r * 255);
    buffer.pixels[index].g = static_cast<uint8_t>(g * 255);
    buffer.pixels[index].b = static_cast<uint8_t>(b * 255);
    buffer.pixels[index].a = static_cast<uint8_t>(a * 255);
    buffer.zbuffer[index] = v.z;
}

// Rasterization functions
void drawLine(const Vertex& start, const Vertex& end, int axis,
             const function<void(const Vertex&)>& callback) {
    Vertex a = start;
    Vertex b = end;
    
    if (axis == 0 && a.x > b.x) swap(a, b);
    if (axis == 1 && a.y > b.y) swap(a, b);

    double start_val = (axis == 0) ? a.x : a.y;
    double end_val = (axis == 0) ? b.x : b.y;
    
    if (start_val == end_val) return;

    Vertex delta = vertexScale(vertexSubtract(b, a), 1.0 / (end_val - start_val));
    
    double curr_val = ceil(start_val);
    Vertex curr = vertexAdd(a, vertexScale(delta, (curr_val - start_val)));

    while (curr_val < end_val) {
        callback(curr);
        curr = vertexAdd(curr, delta);
        curr_val += 1.0;
    }
}

void drawTriangle(const Vertex& p, const Vertex& q, const Vertex& r,
                 const function<void(const Vertex&)>& callback) {
    vector<Vertex> vertices = {p, q, r};
    sort(vertices.begin(), vertices.end(),
         [](const Vertex& a, const Vertex& b) { return a.y > b.y; });
    
    vector<Vertex> tm_edges, tb_edges, mb_edges;
    
    drawLine(vertices[0], vertices[1], 1, 
            [&tm_edges](const Vertex& v) { tm_edges.push_back(v); });
    drawLine(vertices[1], vertices[2], 1, 
            [&mb_edges](const Vertex& v) { mb_edges.push_back(v); });
    drawLine(vertices[0], vertices[2], 1, 
            [&tb_edges](const Vertex& v) { tb_edges.push_back(v); });

    reverse(tm_edges.begin(), tm_edges.end());
    reverse(mb_edges.begin(), mb_edges.end());
    reverse(tb_edges.begin(), tb_edges.end());

    for (size_t i = 0; i < tm_edges.size(); i++) {
        drawLine(tm_edges[i], tb_edges[i], 0, callback);
    }
    
    for (size_t i = 0, j = tm_edges.size(); i < mb_edges.size(); i++, j++) {
        if (j >= tb_edges.size()) break;
        drawLine(mb_edges[i], tb_edges[j], 0, callback);
    }
}

void renderTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3,
                   Buffer& buffer, const vector<double>& transform,
                   bool enableHyp) {
    Vertex p = v1, q = v2, r = v3;
    if (!transform.empty()) {
        p = transformVertex(p, transform);
        q = transformVertex(q, transform);
        r = transformVertex(r, transform);
    }

    p = normalizeVertex(p, buffer.width, buffer.height, enableHyp);
    q = normalizeVertex(q, buffer.width, buffer.height, enableHyp);
    r = normalizeVertex(r, buffer.width, buffer.height, enableHyp);

    drawTriangle(p, q, r, [&buffer](const Vertex& v) {
        setPixel(buffer, v);
    });
}

// File parsing and main rendering function
void parseAndRender(const char* filename, Image& img) {
    ifstream file(filename);
    string line;
    
    vector<Vertex> vertices;
    vector<array<double, 3>> colors;
    Buffer buffer = createBuffer(img.width(), img.height());
    vector<double> currentTransform;
    
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "position") {
            int count;
            iss >> count;
            vertices.clear();
            
            for (int i = 0; i < count; i++) {
                Vertex v;
                iss >> v.x >> v.y >> v.z >> v.w;
                vertices.push_back(v);
            }
        }
        else if (command == "color") {
            int count;
            iss >> count;
            colors.clear();
            
            for (int i = 0; i < count; i++) {
                array<double, 3> color;
                iss >> color[0] >> color[1] >> color[2];
                colors.push_back(color);
            }
            
            for (size_t i = 0; i < min(vertices.size(), colors.size()); i++) {
                vertices[i].r = colors[i][0];
                vertices[i].g = colors[i][1];
                vertices[i].b = colors[i][2];
                vertices[i].a = 1.0;
            }
        }
        else if (command == "drawArraysTriangles") {
            int start, count;
            iss >> start >> count;
            
            for (int i = start; i < start + count - 2; i += 3) {
                if (i + 2 < vertices.size()) {
                    renderTriangle(
                        vertices[i],
                        vertices[i + 1],
                        vertices[i + 2],
                        buffer,
                        currentTransform,
                        true
                    );
                }
            }
        }
    }
    
    // Copy buffer to image
    for (uint32_t y = 0; y < buffer.height; y++) {
        for (uint32_t x = 0; x < buffer.width; x++) {
            size_t index = y * buffer.width + x;
            img[y][x].r = buffer.pixels[index].r;
            img[y][x].g = buffer.pixels[index].g;
            img[y][x].b = buffer.pixels[index].b;
            img[y][x].a = buffer.pixels[index].a;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input file>" << endl;
        return 1;
    }

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

    Image img(width, height);
    cout << "Creating image: " << width << " x " << height << " -> " << outputFilename << endl;
    
    parseAndRender(argv[1], img);
    img.save(outputFilename.c_str());

    return 0;
}