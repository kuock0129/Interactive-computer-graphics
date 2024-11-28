#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstring>

#include "uselibpng.h"  // Include the custom PNG library header

using namespace std;
using uc = unsigned char;

// Forward declarations to resolve circular dependencies
class Vector3f;
class Vector4f;
class Material;

class Vector3f {
public:
    // Constructors
    Vector3f() : x(0), y(0), z(0) {}
    Vector3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    // Accessor methods
    float& operator[](int index) {
        return (index == 0) ? x : (index == 1) ? y : z;
    }
    const float& operator[](int index) const {
        return (index == 0) ? x : (index == 1) ? y : z;
    }

    // Vector addition
    Vector3f operator+(const Vector3f& other) const {
        return Vector3f(x + other.x, y + other.y, z + other.z);
    }

    // Vector subtraction
    Vector3f operator-(const Vector3f& other) const {
        return Vector3f(x - other.x, y - other.y, z - other.z);
    }

    Vector3f& operator+=(const Vector3f& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    // Scalar multiplication
    Vector3f operator*(float scalar) const {
        return Vector3f(x * scalar, y * scalar, z * scalar);
    }

    // Vector multiplication
    Vector3f operator*(const Vector3f& other) const {
        return Vector3f(x * other.x, y * other.y, z * other.z);
    }

    // Negate vector
    void negate() {
        x = -x;
        y = -y;
        z = -z;
    }

    // Clamp values
    void clamp(float min_val, float max_val) {
        x = std::max(min_val, std::min(max_val, x));
        y = std::max(min_val, std::min(max_val, y));
        z = std::max(min_val, std::min(max_val, z));
    }

    // Dot product (static method)
    static float dot(const Vector3f& a, const Vector3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    // Magnitude/Length
    float abs() const {
        return sqrt(x * x + y * y + z * z);
    }

    // Squared magnitude
    float absSquared() const {
        return x * x + y * y + z * z;
    }

    // Normalize the vector
    void normalize() {
        float length = abs();
        if (length > 0) {
            x /= length;
            y /= length;
            z /= length;
        }
    }

    // Output stream operator
    friend ostream& operator<<(ostream& os, const Vector3f& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }

    // Member variables
    float x, y, z;
};



// Vector4f class with integrated operations
class Vector4f {
public:
    // Constructors
    Vector4f() : x(0), y(0), z(0), w(0) {}
    Vector4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vector4f(const Vector3f& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    // Accessor methods
    float& operator[](int index) {
        return (index == 0) ? x : (index == 1) ? y : (index == 2) ? z : w;
    }
    const float& operator[](int index) const {
        return (index == 0) ? x : (index == 1) ? y : (index == 2) ? z : w;
    }

    // Vector addition
    Vector4f operator+(const Vector4f& other) const {
        return Vector4f(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    // Scalar multiplication
    Vector4f operator*(float scalar) const {
        return Vector4f(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    // Output stream operator
    friend ostream& operator<<(ostream& os, const Vector4f& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
        return os;
    }

    // Member variables
    float x, y, z, w;
};

// Use Vector4f as Color
using Color = Vector4f;
using Color3 = Vector3f;
using Color4 = Vector4f;





// Modified Hit struct
struct Hit {
    float t;
    Material* material;
    Vector3f normal;
};




// Light base class
class Light {
public:
    Light() {}
    virtual ~Light() {}
    virtual void getIllumination(const Vector3f& p, Vector3f& dir,
        Vector3f& col, float& distanceToLight) const = 0;
};

// SunLight derived class
class SunLight : public Light {
public:
    SunLight(const Vector3f& d, const Vector3f& c) : dir_to_light(d), color(c) {}
    SunLight() = delete;
    
    void getIllumination(const Vector3f& p, Vector3f& dir,
        Vector3f& col, float& distanceToLight) const override {
        dir = dir_to_light;
        col = color;
        distanceToLight = -1;
    }

private:
    Vector3f dir_to_light;
    Vector3f color;
};




// Ray class
class Ray {
public:
    Ray(const Vector3f& orig, const Vector3f& dir, int d = 0) {
        origin = orig;
        direction = dir;
        depth = d;
    }

    Ray(const Ray& r) {
        origin = r.origin;
        direction = r.direction;
        depth = r.depth;
    }

    const Vector3f& getOrigin() const { return origin; }
    const Vector3f& getDirection() const { return direction; }

    Vector3f pointAtParameter(float t) const {
        return origin + direction * t;
    }

private:
    Vector3f origin;
    Vector3f direction;
    int depth;
};



// Material class definition
class Material {
public:
    Material(float r = 0, float g = 0, float b = 0) 
        : diffusion_color(r, g, b), use_texture(false) {}
    Material(const Color3& color) 
        : Material(color[0], color[1], color[2]) {}
    ~Material() {}

    Vector3f& getDiffusionColor() { return diffusion_color; }

    Vector3f Shade(const Ray& ray, const Hit& hit,
        const Vector3f& dir_to_light, const Vector3f& light_color) {
        Vector3f n = hit.normal;
        // Make sure normal faces toward the viewer
        if (Vector3f::dot(n, ray.getDirection()) > 0) {
            n.negate();
        }
        // Calculate lighting
        // float lamport = std::max(Vector3f::dot(n, dir_to_light), 0.0f);

        // Don't clamp the dot product to 0, let it be negative
        float n_dot_l = Vector3f::dot(n, dir_to_light);
        float lamport = n_dot_l;
        Vector3f ret = (light_color * diffusion_color) * lamport;
        ret.clamp(0, 1);
        return ret;
    }

    friend std::ostream& operator<<(std::ostream& os, const Material& mat) {
        os << mat.diffusion_color[0] << ',' << mat.diffusion_color[1] << ',' << mat.diffusion_color[2];
        return os;
    }

private:
    Color3 diffusion_color;
    bool use_texture;
};


// Modified Sphere class
class Sphere {
public:
    Sphere(float rad, const Vector3f& cen, Material* mat)
        : radius(rad), center(cen), material(mat) {}

    float radius;
    Vector3f center;
    Material* material;

    bool intersect(const Ray& ray, Hit& hit, float tmin = 0) {
        float r2 = radius * radius;
        Vector3f ro = center - ray.getOrigin();
        bool inside = r2 > ro.absSquared();

        float tc = Vector3f::dot(ro, ray.getDirection()) / ray.getDirection().absSquared();
        if (!inside && tc < 0) return false;

        float d2 = (ray.pointAtParameter(tc) - center).absSquared();
        if (!inside && r2 < d2) return false;

        float toffset = sqrt(r2 - d2) / ray.getDirection().abs();

        float t = (inside) ? tc + toffset : tc - toffset;
        if (t < tmin) return false;

        hit.t = t;
        hit.material = material;
        hit.normal = ray.pointAtParameter(t) - center;
        hit.normal.normalize();
        return true;
    }
private:
    // float radius;
    // Vector3f center;
    // Color color;
};





// Scene class
class Scene {
public:
    void addSphere(Sphere&& sphere) {
        // cout << "Adding sphere: " << sphere << endl;
        printf("Adding sphere: %f %f %f %f\n", sphere.radius, sphere.center.x, sphere.center.y, sphere.center.z);
        objects.push_back(std::move(sphere));
    }

    void addLight(Light* light) {
        lights.push_back(light);
    }

    vector<Light*>& getLights() {
        return lights;
    }

    bool intersect(const Ray& ray, Hit& hit, float tmin = 0) {
        bool ret = false;
        for (size_t i = 0; i < objects.size(); i++) {
            Hit curhit;
            if (objects[i].intersect(ray, curhit, tmin)) {
                if (!ret || hit.t > curhit.t) {
                    hit = curhit;
                }
                ret = true;
            }
        }
        return ret;
    }

private:
    vector<Sphere> objects;
    vector<Light*> lights;
};

// Camera class
class Camera {
public:
    Camera() : eye(0, 0, 0), forward(0, 0, -1), right(1, 0, 0), up(0, 1, 0) {}

    Ray generateRay(float sx, float sy) {
        Vector3f dir = forward + right * sx + up * sy;
        dir.normalize();  // Normalize the direction vector
        return Ray(eye, dir, 0);
    }
private:
    Vector3f eye;
    Vector3f forward;
    Vector3f right;
    Vector3f up;
};

// Refactored Picture class using Image
class Picture {
public:
    Picture(const int width, const int height) : _image(width, height) {
        _width = width;
        _height = height;
    }

    void setPixel(int x, int y, const Vector4f& color) {
        assert(x >= 0 && x < _width);
        assert(y >= 0 && y < _height);

        // Convert linear color to 8-bit per channel
        pixel_t& pixel = _image[y][x];
        pixel.r = linear_2_sRGB(color[0]);
        pixel.g = linear_2_sRGB(color[1]);
        pixel.b = linear_2_sRGB(color[2]);
        pixel.a = linear_2_sRGB(color[3]);
    }

    void exportPNG(const char* filename) {
        _image.save(filename);
    }

private:
    Image _image;
    int _width, _height;

    // Linear to sRGB conversion following the official formula
    static inline uc linear_2_sRGB(float linear) {
        float sRGB;
        if (linear <= 0.0031308f) {
            sRGB = 12.92f * linear;
        } else {
            sRGB = 1.055f * pow(linear, 1.0f/2.4f) - 0.055f;
        }
        return round(sRGB * 255.0f);
    }

    // For alpha channel, keep linear conversion
    static inline uc linear_2char(float color) {
        return round(color * 255);
    }
};



// ConfigParser class
class ConfigParser {
public:
    struct Config {
        // Config();
        // ~Config();
        string name;
        int w, h;
        Scene scene;
        Camera camera;
        vector<Material*> materials;


        Config() {
            // create default color
            Material* default_material = new Material(1, 1, 1);
            materials.push_back(default_material);
        }

        ~Config() {
            for (size_t i = 0; i < materials.size(); i++) {
                delete materials[i];
                materials[i] = nullptr;
            }
        }

    };

    int readConfigFromFile(char* filename, Config& config) {
        ifstream fin(filename);
        string line;

        // Color color_stat(0, 0, 0, 1);
        while (getline(fin, line)) {
            vector<string> command;
            parseLineToCommand(line, command);
            if (command.empty()) continue;

            if (command[0] == "png")
                parsePng(command, config);
            else if (command[0] == "sphere")
                parseSphere(command, config);
            else if (command[0] == "color")
                parseColor(command, config);
            else if (command[0] == "sun")
                parseSun(command, config);
        }
        return 0;
    }

private:
    static bool toInt(const string& str, int& num) {
        num = 0;
        size_t i = 0;
        int sign = 1;
        if (str[0] == '-') { sign = -1; i = 1; }
        bool valid = false;
        for (; i < str.size(); ++i) {
            if (isdigit(str[i])) {
                num *= 10;
                num += int(str[i] - '0');
                valid = true;
            }
            else return false;
        }
        num *= sign;
        return valid;
    }

    static size_t getToken(const string& str, string& tok, size_t pos = 0) {
        const string del = " \t";
        size_t begin = str.find_first_not_of(del, pos);
        if (begin == string::npos) { tok = ""; return begin; }
        size_t end = str.find_first_of(del, begin);
        tok = str.substr(begin, end - begin);
        return end;
    }

    static void parseLineToCommand(const string& line, vector<string>& command) {
        string token;
        size_t pos = getToken(line, token);
        while (!token.empty()) {
            command.push_back(token);
            pos = getToken(line, token, pos);
        }
    }

    static vector<float> readFloatArray(const vector<string>& tokens, int start, int len) {
        vector<float> nums;
        for (int i = start; i < start + len; i++) {
            nums.push_back(stof(tokens[i]));
        }
        return nums;
    }   


    // // Config
    // Config::Config() {
    //     // create default color
    //     Material *default_material = new Material(1,1,1);
    //     materials.push_back(default_material);
    // }
    // Config::~Config() {
    //     for (size_t i = 0; i < materials.size(); i++) {
    //         delete materials[i];
    //         materials[i] = nullptr;
    //     }
    // }
    

    int parsePng(const vector<string>& command, Config& config) {
        const int PNG_CMD_SIZE = 4;
        if (command.size() != PNG_CMD_SIZE) return -1;

        int h, w;
        if (!toInt(command[1], w) || !toInt(command[2], h)) return -1;

        config.w = w;
        config.h = h;
        config.name = command[3];
        return 0;
    }

    int parseSphere(const vector<string> &command, Config &config) {
        const int SPHERE_CMD_SIZE = 5;
        if (command.size() != SPHERE_CMD_SIZE) return -1;

        vector<float> data = readFloatArray(command, 1, 4);
        // Sphere sphere(data[3], Vector3f(data[0], data[1], data[2]), color_stat);
        Material *mat = config.materials.back();
        Sphere sphere(data[3], Vector3f(data[0],data[1],data[2]), mat);
        config.scene.addSphere(std::move(sphere));
        return 0;
    }

    int parseColor(const vector<string>& command, Config &config) {
        vector<float> data = readFloatArray(command, 1, 3);
        // color = Color(data[0], data[1], data[2], 1.0f);
        Material *mat = new Material(data[0], data[1], data[2]);
        config.materials.push_back(mat);
 
        return 0;
    }

    int parseSun(const vector<string>& command, Config& config) {
        vector<float> data = readFloatArray(command, 1, 3); // direction
        Vector3f dir_to_light = Vector3f(data[0], data[1], data[2]);
        dir_to_light.normalize();
        Light *nlight = new SunLight(dir_to_light, config.materials.back()->getDiffusionColor());
        config.scene.addLight(nlight);
        return 0;
    }
};







void printDebugInfo(const Ray& ray, const Hit& hit, const Vector3f& dir_to_light, 
                    const Vector3f& light_color, const Vector3f& final_color) {
    // Ray information
    std::cout << "Ray origin: " << ray.getOrigin() << std::endl;
    std::cout << "Ray direction: " << ray.getDirection() << std::endl;
    
    // Intersection information
    std::cout << "Intersection depth: " << hit.t << std::endl;
    Vector3f intersectionPoint = ray.pointAtParameter(hit.t);
    std::cout << "Intersection point: " << intersectionPoint << std::endl;
    std::cout << "Surface normal: " << hit.normal << std::endl;
    
    // Light information
    std::cout << "Sun direction: " << dir_to_light << std::endl;
    
    // Shading calculations
    // Shading calculations - show raw dot product before clamping
    float lambert = Vector3f::dot(hit.normal, dir_to_light);
    std::cout << "Lambert dot product: " << lambert << std::endl;
    
    
    // Color calculations
    std::cout << "Linear color: " << final_color << std::endl;

    // Proper sRGB conversion
    auto linear_2_sRGB = [](float linear) {
        float sRGB;
        if (linear <= 0.0031308f) {
            sRGB = 12.92f * linear;
        } else {
            sRGB = 1.055f * pow(linear, 1.0f/2.4f) - 0.055f;
        }
        // cout << sRGB << endl;
        return sRGB * 255.0f;
    };
    
    // sRGB conversion (assuming linear to sRGB conversion)
    int r = linear_2_sRGB(final_color.x);
    int g = linear_2_sRGB(final_color.y);
    int b = linear_2_sRGB(final_color.z);
    std::cout << "sRGB color: (" << r << ", " << g << ", " << b << ")" << std::endl;
}






int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <config_file>" << endl;
        return -1;
    }

    ConfigParser::Config config;
    ConfigParser configparser;
    configparser.readConfigFromFile(argv[1], config);

    Picture image(config.w, config.h);
    Camera& camera = config.camera;
    Scene& scene = config.scene;

    for (int i = 0; i < config.w; i++) {
        for (int j = 0; j < config.h; j++) {
            float side = max(config.w, config.h);
            float sx = ((float)i * 2 - config.w) / side;
            float sy = ((float)config.h - 2 * j) / side;

            Ray ray = camera.generateRay(sx, sy);
            Hit hit;

            Color4 pixel_color(0, 0, 0, 0);
            if (scene.intersect(ray, hit)) {

                // 
                vector<Light*>& lights = scene.getLights();
                Vector3f rgb;
                for (size_t l = 0; l < lights.size(); l++) {
                    Vector3f light_color;
                    Vector3f dir_to_light;
                    float d;
                    Vector3f p_hit = ray.pointAtParameter(hit.t);
                    lights[l]->getIllumination(p_hit,
                        dir_to_light, light_color, d);
                        
                    Ray secondary(p_hit, dir_to_light);
                    float epsilon = 0.0001f; // magic number
                    Hit second_hit;
                    if (scene.intersect(secondary, second_hit, epsilon)) {
                        light_color = Vector3f(0,0,0);
                    }

                    // rgb += hit.material->Shade(ray, hit, dir_to_light, light_color);
                    rgb += hit.material->Shade(ray, hit, dir_to_light, light_color);
                    if (i == 82 && j == 70){
                        printDebugInfo(ray, hit, dir_to_light, light_color, rgb);
                    }
                }
                pixel_color = Vector4f(rgb, 1.0f);
            }

            image.setPixel(i, j, pixel_color);
        }
    }

    image.exportPNG(config.name.c_str());
    return 0;
}