#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <functional>
#include "uselibpng.h"

using namespace std;

class Vector {
public:
    double x, y, z;

    Vector(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    Vector operator+(const Vector& v) const {
        return Vector(x + v.x, y + v.y, z + v.z);
    }

    Vector operator-(const Vector& v) const {
        return Vector(x - v.x, y - v.y, z - v.z);
    }

    Vector operator*(double scalar) const {
        return Vector(x * scalar, y * scalar, z * scalar);
    }

    double dot(const Vector& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    Vector normalize() const {
        double mag = sqrt(x * x + y * y + z * z);
        return Vector(x / mag, y / mag, z / mag);
    }
};

class Color {
public:
    double r, g, b;

    Color(double r = 0, double g = 0, double b = 0) : r(r), g(g), b(b) {}

    Color operator+(const Color& c) const {
        return Color(r + c.r, g + c.g, b + c.b);
    }

    Color operator*(double scalar) const {
        return Color(r * scalar, g * scalar, b * scalar);
    }

    Color operator*(const Color& c) const {
        return Color(r * c.r, g * c.g, b * c.b);
    }

    Color clamp() const {
        return Color(min(1.0, max(0.0, r)), min(1.0, max(0.0, g)), min(1.0, max(0.0, b)));
    }

    Color toSRGB() const {
        auto linearToSRGB = [](double c) {
            return (c <= 0.0031308) ? 12.92 * c : 1.055 * pow(c, 1.0 / 2.4) - 0.055;
        };
        return Color(linearToSRGB(r), linearToSRGB(g), linearToSRGB(b)).clamp();
    }
};

class Sphere {
public:
    Vector center;
    double radius;
    Color color;

    Sphere(const Vector& center, double radius, const Color& color)
        : center(center), radius(radius), color(color) {}
};

class Light {
public:
    Vector position;
    Color color;

    Light(const Vector& position, const Color& color)
        : position(position), color(color) {}
};

class Ray {
public:
    Vector origin;
    Vector direction;

    Ray(const Vector& origin, const Vector& direction)
        : origin(origin), direction(direction.normalize()) {}
};

bool intersectRaySphere(const Ray& ray, const Sphere& sphere, double& t) {
    Vector oc = ray.origin - sphere.center;
    double a = ray.direction.dot(ray.direction);
    double b = 2.0 * oc.dot(ray.direction);
    double c = oc.dot(oc) - sphere.radius * sphere.radius;
    double discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return false;
    }
    
    double t0 = (-b - sqrt(discriminant)) / (2.0 * a);
    double t1 = (-b + sqrt(discriminant)) / (2.0 * a);
    
    // Return the nearest positive intersection
    if (t0 > 0) {
        t = t0;
        return true;
    }
    if (t1 > 0) {
        t = t1;
        return true;
    }
    return false;
}

Color traceRay(const Ray& ray, const vector<Sphere>& spheres, const Light& light) {
    double t_min = INFINITY;
    const Sphere* hit_sphere = nullptr;
    
    for (const auto& sphere : spheres) {
        double t;
        if (intersectRaySphere(ray, sphere, t) && t > 0 && t < t_min) {
            t_min = t;
            hit_sphere = &sphere;
        }
    }
    
    if (hit_sphere) {
        Vector hit_point = ray.origin + ray.direction * t_min;
        Vector normal = (hit_point - hit_sphere->center).normalize();
        Vector light_dir = (light.position - hit_point).normalize();
        
        // Ambient lighting
        Color ambient = hit_sphere->color * 0.1;
        
        // Diffuse lighting
        double diffuse = max(0.0, normal.dot(light_dir));
        Color diffuse_color = hit_sphere->color * light.color * diffuse;
        
        // Simple shadows
        Ray shadow_ray(hit_point + normal * 0.001, light_dir);
        bool in_shadow = false;
        for (const auto& sphere : spheres) {
            double shadow_t;
            if (intersectRaySphere(shadow_ray, sphere, shadow_t) && shadow_t > 0) {
                in_shadow = true;
                break;
            }
        }
        
        return in_shadow ? ambient : ambient + diffuse_color;
    }
    
    return Color(0, 0, 0); // Background color
}

void renderScene(Image& img, const vector<Sphere>& spheres, const Light& light) {
    Vector eye(0, 0, 0);
    Vector forward(0, 0, -1);
    Vector right(1, 0, 0);
    Vector up(0, 1, 0);

    double aspect_ratio = static_cast<double>(img.width()) / img.height();
    double fov = 90.0;
    double scale = tan(fov * 0.5 * M_PI / 180.0);

    for (uint32_t y = 0; y < img.height(); y++) {
        for (uint32_t x = 0; x < img.width(); x++) {
            double px = (2.0 * (x + 0.5) / img.width() - 1.0) * scale * aspect_ratio;
            double py = (1.0 - 2.0 * (y + 0.5) / img.height()) * scale;

            Vector direction = (forward + right * px + up * py).normalize();
            Ray ray(eye, direction);
            Color pixel_color = traceRay(ray, spheres, light);
            // Color srgb = pixel_color.toSRGB();

            // img[y][x].r = static_cast<uint8_t>(255 * srgb.r);
            // img[y][x].g = static_cast<uint8_t>(255 * srgb.g);
            // img[y][x].b = static_cast<uint8_t>(255 * srgb.b);

            if (pixel_color.r == 0 && pixel_color.g == 0 && pixel_color.b == 0) {
                img[y][x].a = 0; // Transparent
            } else {
                img[y][x].a = 255; // Opaque
            }
        }
    }
}


void parseInputFile(const string& filename, uint32_t& width, uint32_t& height, 
                   string& outputFilename, vector<Sphere>& spheres) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open input file: " + filename);
    }

    string line;
    Color currentColor(1, 1, 1); // Default color is white

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        istringstream iss(line);
        string keyword;
        iss >> keyword;

        if (keyword == "png") {
            iss >> width >> height >> outputFilename;
        } else if (keyword == "color") {
            double r, g, b;
            if (iss >> r >> g >> b) {
                currentColor = Color(r, g, b);
            }
        } else if (keyword == "sphere") {
            double x, y, z, radius;
            if (iss >> x >> y >> z >> radius) {
                spheres.emplace_back(Vector(x, y, z), radius, currentColor);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            cerr << "Usage: " << argv[0] << " <input file>" << endl;
            return 1;
        }

        uint32_t width = 0, height = 0;
        string outputFilename;
        vector<Sphere> spheres;
        
        parseInputFile(argv[1], width, height, outputFilename, spheres);
        
        if (width == 0 || height == 0 || outputFilename.empty()) {
            throw runtime_error("Invalid PNG parameters in input file");
        }

        cout << "Creating image: " << width << " x " << height << " -> " << outputFilename << endl;
        
        Image img(width, height);
        Light light(Vector(5, 5, -10), Color(1, 1, 1));
        renderScene(img, spheres, light);
        img.save(outputFilename.c_str());

        cout << "Image saved to " << outputFilename << endl;
        return 0;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}