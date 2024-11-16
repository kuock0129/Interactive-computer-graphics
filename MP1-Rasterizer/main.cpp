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

int width, height;
string outputFilename;

class Vertex {
public:
    double x{0}, y{0}, z{0}, w{1.0}; // Position
    double r{0}, g{0}, b{0}, a{1.0}; // Color
    double s{0}, t{0};      // Texture coordinates
    
    // Applies a 4x4 transformation matrix to the vertex.
    Vertex& transform(const vector<double>& matrix) {
        vector<double> result(4, 0.0);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result[i] += matrix[i * 4 + j] * getValue(j);
            }
        }
        x = result[0]; y = result[1]; z = result[2]; w = result[3];
        return *this;
    }

    // Normalizes the vertex position to screen coordinates.
    // Converts a vertex to normalized device coordinates and then maps them to screen space.
    Vertex normalized(int width, int height, bool enableHyp) const {
        Vertex result = *this;
        if (w != 0) {
            result.x /= w;
            result.y /= w;
            result.z /= w;
            if (enableHyp) {
                result.r /= w;
                result.g /= w;
                result.b /= w;
                result.a /= w;
                result.s /= w;
                result.t /= w;
            }
        }
        result.x = (result.x + 1) * width / 2;
        result.y = (result.y + 1) * height / 2;
        return result;
    }

    // Implements basic arithmetic operations (addition, subtraction, scaling) for vertices.
    Vertex operator-(const Vertex& other) const {
        Vertex result;
        result.x = x - other.x;
        result.y = y - other.y;
        result.z = z - other.z;
        result.w = w - other.w;
        result.r = r - other.r;
        result.g = g - other.g;
        result.b = b - other.b;
        result.a = a - other.a;
        result.s = s - other.s;
        result.t = t - other.t;
        return result;
    }

    Vertex operator+(const Vertex& other) const {
        Vertex result;
        result.x = x + other.x;
        result.y = y + other.y;
        result.z = z + other.z;
        result.w = w + other.w;
        result.r = r + other.r;
        result.g = g + other.g;
        result.b = b + other.b;
        result.a = a + other.a;
        result.s = s + other.s;
        result.t = t + other.t;
        return result;
    }

    Vertex operator/(double div) const {
        if (div == 0) return Vertex();
        Vertex result;
        result.x = x / div;
        result.y = y / div;
        result.z = z / div;
        result.w = w / div;
        result.r = r / div;
        result.g = g / div;
        result.b = b / div;
        result.a = a / div;
        result.s = s / div;
        result.t = t / div;
        return result;
    }

    Vertex operator*(double mul) const {
        Vertex result;
        result.x = x * mul;
        result.y = y * mul;
        result.z = z * mul;
        result.w = w * mul;
        result.r = r * mul;
        result.g = g * mul;
        result.b = b * mul;
        result.a = a * mul;
        result.s = s * mul;
        result.t = t * mul;
        return result;
    }

private:
    double getValue(int index) const {
        switch(index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
            default: return 0;
        }
    }
};


// Handles texture sampling:
class Texture {
public:
    explicit Texture(const string& filename) : image(filename.c_str()) {}
    // Reads a texture image from a file.
    void sample(double s, double t, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {
        s = fmod(s, 1.0);
        if (s < 0) s += 1.0;
        t = fmod(t, 1.0);
        if (t < 0) t += 1.0;

        int x = static_cast<int>(s * (image.width() - 1) + 0.5);
        int y = static_cast<int>(t * (image.height() - 1) + 0.5);
        
        r = image[y][x].r;
        g = image[y][x].g;
        b = image[y][x].b;
        a = image[y][x].a;
    }

private:
    Image image;
};


// Manages the image and a depth buffer:
class RenderBuffer {
public:
    RenderBuffer(uint32_t width, uint32_t height)
        : image(width, height), zbuffer(width * height, INFINITY) {}

    // setPixel: Writes a pixel to the buffer while performing depth testing and alpha blending.
    void setPixel(const Vertex& v, const Texture* texture = nullptr) {
        int x = static_cast<int>(v.x);
        int y = static_cast<int>(v.y);
        
        if (x < 0 || x >= static_cast<int>(image.width()) || 
            y < 0 || y >= static_cast<int>(image.height())) {
            return;
        }

        size_t index = y * image.width() + x;
        if (useDepthTest && zbuffer[index] <= v.z) {
            return;
        }

        double r = v.r, g = v.g, b = v.b, a = v.a;
        if (texture) {
            uint8_t tr, tg, tb, ta;
            texture->sample(v.s, v.t, tr, tg, tb, ta);
            r = tr / 255.0;
            g = tg / 255.0;
            b = tb / 255.0;
            a = ta / 255.0;
        }

        if (a < 1.0) {
            double existingA = image[y][x].a / 255.0;
            double blendedA = a + existingA * (1 - a);
            if (blendedA > 0) {
                r = (r * a + (image[y][x].r / 255.0) * existingA * (1 - a)) / blendedA;
                g = (g * a + (image[y][x].g / 255.0) * existingA * (1 - a)) / blendedA;
                b = (b * a + (image[y][x].b / 255.0) * existingA * (1 - a)) / blendedA;
                a = blendedA;
            }
        }

        image[y][x].r = static_cast<uint8_t>(r * 255);
        image[y][x].g = static_cast<uint8_t>(g * 255);
        image[y][x].b = static_cast<uint8_t>(b * 255);
        image[y][x].a = static_cast<uint8_t>(a * 255);
        zbuffer[index] = v.z;
    }
    // Saves the rendered image to a file.
    void save(const string& filename) const {
        image.save(filename.c_str());
    }
    // Enables depth testing.
    void enableDepthTest() { useDepthTest = true; }
    void enablesRGB() { usesRGB = true; }
    
    uint32_t width() const { return image.width(); }
    uint32_t height() const { return image.height(); }

    const Image& getImage() const { return image; }

private:
    Image image;
    vector<double> zbuffer;
    bool useDepthTest{false};
    bool usesRGB{false};
};




class Rasterizer {
public:
    //DDA (Digital Differential Analyzer): Draws lines between two vertices by interpolating values linearly.
    void DDA(const Vertex& a, const Vertex& b, int axis,
             const function<void(const Vertex&)>& callback) {
        Vertex start = a;
        Vertex end = b;
        
        if (axis == 0 && start.x > end.x) swap(start, end);
        if (axis == 1 && start.y > end.y) swap(start, end);

        double start_val = (axis == 0) ? start.x : start.y;
        double end_val = (axis == 0) ? end.x : end.y;
        
        if (start_val == end_val) return;

        Vertex delta = (end - start) / (end_val - start_val);
        
        double curr_val = ceil(start_val);
        Vertex curr = start + (delta * (curr_val - start_val));

        while (curr_val < end_val) {
            callback(curr);
            curr = curr + delta;
            curr_val += 1.0;
        }
    }
    //DDATriangle: Rasterizes a triangle by breaking it into scanlines.
    void DDATriangle(const Vertex& p, const Vertex& q, const Vertex& r,
                    const function<void(const Vertex&)>& callback) {
        vector<Vertex> vertices = {p, q, r};
        sort(vertices.begin(), vertices.end(),
             [](const Vertex& a, const Vertex& b) { return a.y > b.y; });
        
        const Vertex& top = vertices[0];
        const Vertex& mid = vertices[1];
        const Vertex& bottom = vertices[2];

        vector<Vertex> tm_edges, tb_edges, mb_edges;
        
        DDA(top, mid, 1, [&tm_edges](const Vertex& v) { tm_edges.push_back(v); });
        DDA(mid, bottom, 1, [&mb_edges](const Vertex& v) { mb_edges.push_back(v); });
        DDA(top, bottom, 1, [&tb_edges](const Vertex& v) { tb_edges.push_back(v); });

        reverse(tm_edges.begin(), tm_edges.end());
        reverse(mb_edges.begin(), mb_edges.end());
        reverse(tb_edges.begin(), tb_edges.end());

        for (size_t i = 0; i < tm_edges.size(); i++) {
            DDA(tm_edges[i], tb_edges[i], 0, callback);
        }
        
        for (size_t i = 0, j = tm_edges.size(); i < mb_edges.size(); i++, j++) {
            if (j >= tb_edges.size()) break;
            DDA(mb_edges[i], tb_edges[j], 0, callback);
        }
    }

    void drawTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3,
                     RenderBuffer& buffer, const vector<double>& transform,
                     bool enableHyp, const Texture* texture = nullptr) {
        Vertex p = v1, q = v2, r = v3;
        if (!transform.empty()) {
            p.transform(transform);
            q.transform(transform);
            r.transform(transform);
        }

        p = p.normalized(buffer.width(), buffer.height(), enableHyp);
        q = q.normalized(buffer.width(), buffer.height(), enableHyp);
        r = r.normalized(buffer.width(), buffer.height(), enableHyp);

        DDATriangle(p, q, r, [&buffer, texture](const Vertex& v) {
            buffer.setPixel(v, texture);
        });
    }
};

void parseInputFile(const char* filename, Image& img) {
    ifstream file(filename);
    string line;
    
    vector<Vertex> vertices;
    vector<array<double, 3>> colors;
    RenderBuffer buffer(img.width(), img.height());
    Rasterizer rasterizer;
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
                    rasterizer.drawTriangle(
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
    
    for (uint32_t y = 0; y < buffer.height(); y++) {
        for (uint32_t x = 0; x < buffer.width(); x++) {
            img[y][x] = buffer.getImage()[y][x];
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
    
    parseInputFile(argv[1], img);
    img.save(outputFilename.c_str());

    return 0;
}