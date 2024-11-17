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
vector<int> elements;
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
            // Normalize position components.
            result.x /= w;
            result.y /= w;
            result.z /= w;

            // Store 1.0 / w for later interpolation use.
            result.w = 1.0 / w;

            // Optionally normalize additional attributes.
            if (enableHyp) {
                // result.r /= w;
                // result.g /= w;
                // result.b /= w;
                // result.a /= w;
                result.s /= w;
                result.t /= w;
            }

            // Map x and y to screen space.
            result.x = (result.x + 1) * width / 2;
            result.y = (result.y + 1) * height / 2;
        } else {
            // Handle w == 0 case explicitly if needed (e.g., set to default or invalid state).
            result = Vertex(); // Reset or handle invalid case appropriately.
        }

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
        : image(width, height), zbuffer(width * height, INFINITY), minDepth(0.0), maxDepth(1.0) {}


    // setPixel: Writes a pixel to the buffer while performing depth testing and alpha blending.
    void setPixel(const Vertex& v, const Texture* texture = nullptr) {
        // cout << "Setting pixel: " << v.x << " " << v.y << " " << v.z << " " << v.r << " " << v.g << " " << v.b << " " << v.a << endl;
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
            // cout << "Alpha blending: " << r << " " << g << " " << b << " " << a << endl;
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

        // // Normalize the depth value to [0, 1]
        // double normalizedDepth = (v.z - minDepth) / (maxDepth - minDepth);
        // normalizedDepth = std::max(0.0, std::min(1.0, normalizedDepth)); // Clamp to [0, 1]

        // // Map the normalized depth to a grayscale color
        // uint8_t gray = static_cast<uint8_t>(normalizedDepth * 255);

        // image[y][x].r = gray;
        // image[y][x].g = gray;
        // image[y][x].b = gray;
        // image[y][x].a = 255; // Fully opaque
        // zbuffer[index] = v.z;
    }

    // Helper function to convert linear RGB to sRGB
    float linearToSrgb(float value) {
        value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);

        if (value <= 0.0031308f) {
            return 12.92f * value;
        } else {
            return 1.055f * pow(value, 1.0f / 2.4f) - 0.055f;
        }
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
    double minDepth;
    double maxDepth;
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
        cout << "Drawing triangle: " << p.x << " " << p.y << "    " << "color:" <<  p.r << " " << p.g << " " << p.b << endl;
        cout << "Drawing triangle: " << q.x << " " << q.y << "    " << "color:" << q.r << " " << q.g << " " << q.b << endl;
        cout << "Drawing triangle: " << r.x << " " << r.y << "    " << "color:" << r.r << " " << r.g << " " << r.b << endl;
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
        cout << "Drawing triangleAAA: " << p.x << " " << p.y << "    " << "color:" <<  p.r << " " << p.g << " " << p.b << endl;
        if (!transform.empty()) {
            p.transform(transform);
            q.transform(transform);
            r.transform(transform);
        }
        cout << "Drawing triangleBBB: " << p.x << " " << p.y << "    " << "color:" <<  p.r << " " << p.g << " " << p.b << endl;
        
        p = p.normalized(buffer.width(), buffer.height(), enableHyp);
        q = q.normalized(buffer.width(), buffer.height(), enableHyp);
        r = r.normalized(buffer.width(), buffer.height(), enableHyp);
        cout << "Drawing triangleCCC: " << p.x << " " << p.y << "    " << "color:" <<  p.r << " " << p.g << " " << p.b << endl;

        DDATriangle(p, q, r, [&buffer, texture](const Vertex& v) {
            buffer.setPixel(v, texture);
        });
    }



};



void parseInputFile(const char* filename, Image& img) {
    ifstream file(filename);
    string line;
    
    vector<Vertex> vertices;
    vector<array<double, 4>> colors; // Change to size 4 to accommodate RGBA
    vector<array<double, 4>> positions;
    RenderBuffer buffer(img.width(), img.height());
    Rasterizer rasterizer;
    vector<double> currentTransform;

    // flag to apply sRGB conversion to all colors
    bool usesRGB = false;

    bool useHYP = false;
    
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        istringstream iss(line);
        string command;
        iss >> command;


        if (command == "depth") {
            buffer.enableDepthTest();
            // depthBuffer.resize(height, vector<float>(width, numeric_limits<float>::infinity()));
        }
        else if(command == "sRGB") {
            usesRGB = true;
        }
        else if(command == "hyp") {
            useHYP = true;
        }

        else if (command == "position") {
            positions.clear();
            int size;
            iss >> size;

            while (true) {
                array<double, 4> pos = {0.0, 0.0, 0.0, 1.0}; // Default A = 1.0 (opaque)
                // iss >> v.x >> v.y >> v.z >> v.w;
                // vertices.push_back(v);
                
                if (!(iss >> pos[0] >> pos[1])) break;
                // pos.z = 0.0f;
                // pos.w = 1.0f;
                if (size >= 3 && !(iss >> pos[2])) break;
                if (size == 4 && !(iss >> pos[3])) break;
                positions.push_back(pos);
            }
            // Assign colors to vertices
            // for (size_t i = 0; i < min(vertices.size(), colors.size()); i++) {
            cout << "Parsed " << positions.size() << " positions" << endl;
            // cout << "vertices.size()" << vertices.size() << endl;
            // for (size_t i = 0; i < positions.size(); i++) {
            //     vertices[i].x = positions[i][0];
            //     vertices[i].y = positions[i][1];
            //     vertices[i].z = positions[i][2];
            //     vertices[i].w = positions[i][3];
            //     cout << "Position: " << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << " " << vertices[i].w << endl;
            // }
            
        }



        else if (command == "color") {
            colors.clear();
            int size;
            iss >> size; // Read the size

            if (size != 3 && size != 4) {
                cerr << "Error: Invalid color size " << size << ". Must be 3 (RGB) or 4 (RGBA)." << endl;
                continue;
            }

            // Read colors until the end of the line
            while (iss) {
                array<double, 4> color = {0.0, 0.0, 0.0, 1.0}; // Default A = 1.0 (opaque)
                iss >> color[0] >> color[1] >> color[2]; // Read RGB
                
                if (size == 4) {
                    iss >> color[3]; // Read A if size is 4
                }

                if (usesRGB) {
                    color[0] = buffer.linearToSrgb(color[0]);
                    color[1] = buffer.linearToSrgb(color[1]);
                    color[2] = buffer.linearToSrgb(color[2]);
                    cout << "Converted color to sRGB: " << color[0] << " " << color[1] << " " << color[2] << endl;
                }
                
                // Only add the color if sufficient values were successfully read
                if (iss) {
                    colors.push_back(color);
                }
            }

            // Assign colors to vertices
            // for (size_t i = 0; i < min(vertices.size(), colors.size()); i++) {
            cout << "Parsed " << colors.size() << " colors" << endl;
            cout << "vertices.size()" << vertices.size() << endl;
            // for (size_t i = 0; i < colors.size(); i++) {
            //     vertices[i].r = colors[i][0];
            //     vertices[i].g = colors[i][1];
            //     vertices[i].b = colors[i][2];
            //     vertices[i].a = colors[i][3];
            //     cout << "Color: " << vertices[i].r << " " << vertices[i].g << " " << vertices[i].b << " " << vertices[i].a << endl;
            // }
            
        }
        else if (command == "elements") {
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
        else if (command == "drawArraysTriangles") {
            int start, count;
            iss >> start >> count;

            //START// Combine vertices before creating indices
            vertices.clear();  // Clear previous vertices
            size_t vertexCount = max(positions.size(), colors.size());
            vertices.resize(vertexCount);
            for (size_t i = 0; i < colors.size(); i++) {
                vertices[i].r = colors[i][0];
                vertices[i].g = colors[i][1];
                vertices[i].b = colors[i][2];
                vertices[i].a = colors[i][3];
                cout << "Color: " << vertices[i].r << " " << vertices[i].g << " " << vertices[i].b << " " << vertices[i].a << endl;
            }
            for (size_t i = 0; i < positions.size(); i++) {
                vertices[i].x = positions[i][0];
                vertices[i].y = positions[i][1];
                vertices[i].z = positions[i][2];
                vertices[i].w = positions[i][3];
                cout << "Position: " << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << " " << vertices[i].w << endl;
            }
            ///END // Combine vertices before creating indices
            
            for (int i = start; i < start + count - 2; i += 3) {
                if (i + 2 < vertices.size()) {
                    cout << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << " " << vertices[i].w
                         << " " << vertices[i].r << " " << vertices[i].g << " " << vertices[i].b << " " << vertices[i].a << endl;
                    cout << vertices[i + 1].x << " " << vertices[i + 1].y << " " << vertices[i + 1].z << " " << vertices[i + 1].w
                         << " " << vertices[i + 1].r << " " << vertices[i + 1].g << " " << vertices[i + 1].b << " " << vertices[i + 1].a << endl;
                    cout << vertices[i + 2].x << " " << vertices[i + 2].y << " " << vertices[i + 2].z << " " << vertices[i + 2].w
                         << " " << vertices[i + 2].r << " " << vertices[i + 2].g << " " << vertices[i + 2].b << " " << vertices[i + 2].a << endl;

                    rasterizer.drawTriangle(
                        vertices[i],
                        vertices[i + 1],
                        vertices[i + 2],
                        buffer,
                        currentTransform,
                        useHYP
                    );
                }
            }
        }
        else if (command == "drawElementsTriangles") {
            int count, offset;
            iss >> count >> offset;

            // Clear previous vertices
            vertices.clear();
            size_t vertexCount = max(positions.size(), colors.size());
            vertices.resize(vertexCount);

            // Populate vertices with position and color
            for (size_t i = 0; i < colors.size(); i++) {
                vertices[i].r = colors[i][0];
                vertices[i].g = colors[i][1];
                vertices[i].b = colors[i][2];
                vertices[i].a = colors[i][3];
            }
            for (size_t i = 0; i < positions.size(); i++) {
                vertices[i].x = positions[i][0];
                vertices[i].y = positions[i][1];
                vertices[i].z = positions[i][2];
                vertices[i].w = positions[i][3];
            }

            // Draw triangles based on `elements` array
            for (int i = 0; i < count; i += 3) {
                int index1 = elements[offset + i];
                int index2 = elements[offset + i + 1];
                int index3 = elements[offset + i + 2];

                if (index1 < vertices.size() && index2 < vertices.size() && index3 < vertices.size()) {
                    // Log the triangle vertices
                    cout << "Triangle vertices: " << endl;
                    cout << "  Vertex 1: " << vertices[index1].x << " " << vertices[index1].y << " " << vertices[index1].z << " " << vertices[index1].w
                        << " " << vertices[index1].r << " " << vertices[index1].g << " " << vertices[index1].b << " " << vertices[index1].a << endl;
                    cout << "  Vertex 2: " << vertices[index2].x << " " << vertices[index2].y << " " << vertices[index2].z << " " << vertices[index2].w
                        << " " << vertices[index2].r << " " << vertices[index2].g << " " << vertices[index2].b << " " << vertices[index2].a << endl;
                    cout << "  Vertex 3: " << vertices[index3].x << " " << vertices[index3].y << " " << vertices[index3].z << " " << vertices[index3].w
                        << " " << vertices[index3].r << " " << vertices[index3].g << " " << vertices[index3].b << " " << vertices[index3].a << endl;

                    // Draw the triangle
                    rasterizer.drawTriangle(
                        vertices[index1],
                        vertices[index2],
                        vertices[index3],
                        buffer,
                        currentTransform,
                        useHYP
                    );
                } else {
                    cout << "Warning: Invalid index in elements array" << endl;
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