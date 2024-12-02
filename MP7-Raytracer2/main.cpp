#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm>
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
    // Static member variables for common vectors
    static const Vector3f ZERO;
    static const Vector3f FORWARD;
    static const Vector3f RIGHT;
    static const Vector3f UP;

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

    bool operator==(const Vector3f& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    // Scalar multiplication
    Vector3f operator*(float scalar) const {
        return Vector3f(x * scalar, y * scalar, z * scalar);
    }

    // Vector multiplication
    Vector3f operator*(const Vector3f& other) const {
        return Vector3f(x * other.x, y * other.y, z * other.z);
    }

    // Add cross product static method
    static Vector3f cross(const Vector3f& a, const Vector3f& b) {
        return Vector3f(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
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

    Vector3f normalized() const {
        Vector3f result = *this;
        result.normalize();
        return result;
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

    
    Vector3f operator/(float scalar) const {
        return Vector3f(x / scalar, y / scalar, z / scalar);
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

    Vector3f xyz() const {
        return Vector3f(x, y, z);
    }

    // Member variables
    float x, y, z, w;
};

// Use Vector4f as Color
using Color = Vector4f;
using Color3 = Vector3f;
using Color4 = Vector4f;

const Vector3f Vector3f::ZERO(0.0f, 0.0f, 0.0f);
const Vector3f Vector3f::FORWARD(0.0f, 0.0f, -1.0f);
const Vector3f Vector3f::RIGHT(1.0f, 0.0f, 0.0f);
const Vector3f Vector3f::UP(0.0f, 1.0f, 0.0f);





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
        distanceToLight = INFINITY;
    }
private:
    Vector3f dir_to_light;
    Vector3f color;
};


class BulbLight : public Light{
public:
    BulbLight(const Vector3f &p, const Vector3f &c):
        src(p), color(c) {}


    void getIllumination( const Vector3f& p, Vector3f& dir, 
        Vector3f& col, float& distanceToLight ) const override {
        Vector3f d = src - p;
        dir = d.normalized();
        col = color / d.absSquared(); // the brightness of light decreases
        distanceToLight = d.abs();
    }
private: 
    Vector3f src;
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
        // float lamport = n_dot_l;
        float lamport = std::max(0.0f, n_dot_l);  // Add this clamp
        Vector3f ret = (light_color * diffusion_color) * lamport;
        // value may not be in the range [0, 1]
        // ret.clamp(0, 1);
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




class Object {
public:
    Object(Material *mat): material(mat) {}
    virtual ~Object() {}
    virtual bool intersect(const Ray &ray, Hit &hit, float tmin) = 0;
    friend ostream& operator << (ostream &os, const Object& object) {
        object.serialize(os);
        return os;
    }
    virtual void serialize(ostream &os) const = 0;
protected:
    Material *material;
};






// Modified Sphere class
// class Sphere {
class Sphere : public Object {
public:
    Sphere(float rad, const Vector3f& cen, Material* mat)
        // : radius(rad), center(cen), material(mat) {}
        :Object(mat), radius(rad), center(cen) {}

    float radius;
    Vector3f center;
    // Material* material;

    bool intersect(const Ray& ray, Hit& hit, float tmin) override {
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

   void serialize(ostream& os) const override {
        os << "Sphere(c=(" << center[0] << ',' << center[1] << ',' << center[2]
            << "),r=" << radius 
            << ",mat=(" << material << "))";
    }   

private:
    // float radius;
    // Vector3f center;
    // Color color;
};







class Plane : public Object {
public:
    Plane(const vector<float> &params, Material *mat) : Object(mat),
        coefficients(params[0], params[1], params[2], params[3]) {}

    bool intersect(const Ray &ray, Hit &hit, float tmin) {
        Vector3f n = getNormal();
        // Normalize the normal vector to prevent numerical issues
        n.normalize();

        float rd_dot_n = Vector3f::dot(ray.getDirection(), n);
        if (rd_dot_n == 0) {
            // cout << "cond1" << endl;
            return false;
        }
        Vector3f p = get1Point();
        float t = Vector3f::dot(p - ray.getOrigin(), n) / rd_dot_n;
        if (t < tmin) {
            // cout << "cond2" << t << ' ' << tmin << endl;
            return false;
        }
        hit.t = t;
        hit.normal = n.normalized();
        hit.material = material;
        return true;
    }

    Vector3f getNormal() const {
        return coefficients.xyz();
    }

    Vector3f get1Point() const {
        if (coefficients[0] != 0)
            return Vector3f(-coefficients[3] / coefficients[0], 0.0f, 0.0f);
        else if (coefficients[1] != 0)
            return Vector3f(0.0f, -coefficients[3] / coefficients[1], 0.0f);
        else if (coefficients[2] != 0)
            return Vector3f(0.0f, 0.0f, -coefficients[3] / coefficients[2]);
        return Vector3f::ZERO;
    }

    void serialize(ostream &os) const {
        os << "Plane(" << coefficients[0] << ',' << coefficients[1]
           << ',' << coefficients[2]
           << ',' << coefficients[3] << ")";
    }

private:
    Vector4f coefficients;
};



class Triangle : public Object {
public:
    // Triangle(const vector<Vector3f> &p, Material *mat);
    // bool intersect(const Ray &ray, Hit &hit, float tmin) override;
    // void serialize(ostream &os) const override;

    Triangle(const vector<Vector3f> &p, Material *mat): Object(mat),
        points(p) {
        normal = Vector3f::cross(p[1] - p[0], p[2] - p[0]).normalized();
        Vector3f a1 = Vector3f::cross(p[2]-p[0], normal);
        Vector3f a2 = Vector3f::cross(p[1]-p[0], normal);
        e1 = a1 / Vector3f::dot(a1, p[1] - p[0]);
        e2 = a2 / Vector3f::dot(a2, p[2] - p[0]);
    }


    bool intersect(const Ray &ray, Hit &hit, float tmin) {
        float rd_dot_n = Vector3f::dot(ray.getDirection(), normal);
        if (rd_dot_n == 0) {
            return false;
        }

        const Vector3f &p0 = points[0];
        float t = Vector3f::dot(p0 - ray.getOrigin(), normal) / rd_dot_n;
        if (t < tmin) {
            return false;
        }

        Vector3f p = ray.pointAtParameter(t);

        // check p combination
        float b1 = Vector3f::dot(e1, p - p0);
        float b2 = Vector3f::dot(e2, p - p0);
        float b0 = 1.0f - b1 - b2;
        if ((b0 < 0) || (b0 > 1) || (b1 < 0) || (b1 > 1) || (b2 < 0) || (b2 > 1)) {
            return false;
        }
        hit.t = t;
        hit.normal = normal;
        hit.material = material;
        return true;
    }


    void serialize(ostream& os) const {
        os << "Triangle(" << points[0] << ',' << points[1]
            << ',' << points[2] << ")";
    }



private:
    vector<Vector3f> points;
    Vector3f normal;
    Vector3f e1, e2;
};






// Scene class
class Scene {
public:
    void addObject(Object *object) {
        // cout << "Adding sphere: " << sphere << endl;
        // printf("Adding sphere: %f %f %f %f\n", sphere.center.x, sphere.center.y, sphere.center.z, sphere.radius);
        objects.push_back(object);
    }

    void addLight(Light* light) {
        lights.push_back(light);
    }

    vector<Light*>& getLights() {
        return lights;
    }

    bool intersect(const Ray& ray, Hit& hit, float tmin = 0) {
        if (ray.getDirection() == Vector3f::ZERO) {
            return false;
        }
        bool ret = false;
        // for (size_t i = 0; i < objects.size(); i++) {
        size_t start = 0;
        size_t end = objects.size();
        for (size_t i = start; i < end; i++) {
            Hit curhit;
            if (objects[i]->intersect(ray, curhit, tmin)) {
                if (!ret || hit.t > curhit.t) {
                    hit = curhit;
                }
                ret = true;
            }
        }
        return ret;
    }

private:
    vector<Object*> objects;
    vector<Light*> lights;
};






// Camera class
class Camera {
public:
    // Camera() : eye(0, 0, 0), forward(0, 0, -1), right(1, 0, 0), up(0, 1, 0) {}

    Camera(const Vector3f &e, const Vector3f &f, const Vector3f &u):
        eye(e), 
        forward(f) {
            right = Vector3f::cross(f, u);//.normalized();
            up = Vector3f::cross(right, f);//.normalized();
            right.normalize();
            up.normalize();
        }

    Ray generateRay(float sx, float sy) {
        Vector3f dir = forward + right * sx + up * sy;
        dir.normalize();  // Normalize the direction vector
        return Ray(eye, dir, 0);
    }

    void debugCameraVectors() const {
        // Print normalized vectors
        cout << "Forward (normalized): " << forward << endl;
        cout << "Up (normalized): " << up << endl;
        cout << "Right (normalized): " << right << endl;
        
        // Verify orthogonality
        cout << "Forward·Up: " << Vector3f::dot(forward, up) << endl;
        cout << "Forward·Right: " << Vector3f::dot(forward, right) << endl;
        cout << "Up·Right: " << Vector3f::dot(up, right) << endl;
    }


private:
    Vector3f eye = Vector3f::ZERO;
    Vector3f forward = Vector3f::FORWARD;
    Vector3f right = Vector3f::RIGHT;
    Vector3f up = Vector3f::UP;
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
        // do clamp here
        sRGB = max(0.0f, min(1.0f, sRGB));
        return round(sRGB * 255.0f);
    }

    // For alpha channel, keep linear conversion
    static inline uc linear_2char(float color) {
        color = max(0.0f, min(1.0f, color));
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
        // Camera camera;
        vector<Material*> materials;
        bool do_exposure = false;
        float exposure;
        // camera
        Vector3f eye = Vector3f::ZERO;
        Vector3f forward = Vector3f::FORWARD;
        Vector3f up = Vector3f::UP;
        vector<Vector3f> vertices;
        Camera getCamera() const { return Camera(eye, forward, up); }


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
            else if (command[0] == "expose")
                parseExposure(command, config);
            else if (command[0] == "eye")
                parseEye(command, config);
            else if (command[0] == "forward")
                parseForward(command, config);
            else if (command[0] == "up")
                parseUp(command, config);
            else if (command[0] == "plane")
                parsePlane(command, config);
            else if (command[0] == "xyz")
                parseXYZ(command, config);
            else if (command[0] == "tri")
                parseTRI(command, config);
            else if (command[0] == "bulb")
                parseBulb(command, config);
            }
        return 0;
    }

private:
    vector<Vector3f> vertices; 
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
        
        // Sphere sphere(data[3], Vector3f(data[0],data[1],data[2]), mat);
        // config.scene.addSphere(std::move(sphere));
        // return 0;
        
        // Sphere *sphere = new Sphere(data[3], Vector3f(data[0],data[1],data[2]), mat);
        Object *sphere = new Sphere(data[3], Vector3f(data[0],data[1],data[2]), mat);
        config.scene.addObject(sphere); 
        return 0;
    }

    int parsePlane(const vector<string> &command, Config &config) {
        const int PLANE_CMD_SIZE = 5;
        if (command.size() != PLANE_CMD_SIZE) {
            return -1;
        }
        vector<float> data = readFloatArray(command, 1, 4);
        Material *mat = config.materials.back();
        // Plane *plane = new Plane(data, mat);
        Object *plane = new Plane(data, mat);
        config.scene.addObject(plane); 
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


    int parseExposure(const vector<string> &command, Config &config) {
        float v = stof(command[1]);
        cout << "exposure" << v << endl;
        config.do_exposure = true;
        config.exposure = v;
        return 0;
    }


    int parseEye(const vector<string> &command, Config &config) {
        vector<float> data = readFloatArray(command, 1, 3);
        Vector3f vec(data[0], data[1], data[2]);
        config.eye = vec;
        return 0;
    }
    int parseForward(const vector<string> &command, Config &config) {
        vector<float> data = readFloatArray(command, 1, 3);
        Vector3f vec(data[0], data[1], data[2]);
        config.forward = vec;
        return 0;
    }
    int parseUp(const vector<string> &command, Config &config) {
        vector<float> data = readFloatArray(command, 1, 3);
        Vector3f vec(data[0], data[1], data[2]);
        config.up = vec;
        return 0;
    }


    int parseXYZ(const vector<string> &command, Config &config) {
        vector<float> data = readFloatArray(command, 1, 3);
        vertices.push_back(Vector3f(data[0], data[1], data[2]));
        return 0;
    }

    int parseTRI(const vector<string> &command, Config &config) {
        vector<Vector3f> points;
        for (int i = 1; i < 4; i++) {
            int idx;
            if (!toInt(command[i], idx)) return -1;
            // TODO: the index is somehow wrong
            idx = (idx > 0) ? idx - 1 : (idx + vertices.size()) % vertices.size(); // for minus
            points.push_back(vertices[idx]);
        }
        Material *mat = config.materials.back();
        Object *triangle = new Triangle(points, mat);
        config.scene.addObject(triangle);
        return 0;
    }

    int parseBulb(const vector<string> &command, Config &config) {
        vector<float> data = readFloatArray(command, 1, 3);
        Light *bulb = new BulbLight(Vector3f(data[0],data[1],data[2]), config.materials.back()->getDiffusionColor());
        config.scene.addLight(bulb);
        return 0;
    }

};



static float expose(float l, float v) {
    return 1.0f - exp(-v*l);
}





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
    // final_color = max(0.0f, min(1.0f, final_color));
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
        sRGB = max(0.0f, min(1.0f, sRGB));
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
    Camera camera = config.getCamera();
    camera.debugCameraVectors();  // Call debug function on camera instance
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
                    if (scene.intersect(secondary, second_hit, epsilon) && (second_hit.t < d)) {
                        light_color = Vector3f(0,0,0);
                    }
                    // rgb += hit.material->Shade(ray, hit, dir_to_light, light_color);
                    rgb += hit.material->Shade(ray, hit, dir_to_light, light_color);
                    if (i == 82 && j == 70){
                        cout << "pixel:" << i <<" "<< j<<endl;
                        printDebugInfo(ray, hit, dir_to_light, light_color, rgb);
                    }
                }
                // do exposure
                if (config.do_exposure) {
                    rgb[0] = expose(rgb[0], config.exposure);
                    rgb[1] = expose(rgb[1], config.exposure);
                    rgb[2] = expose(rgb[2], config.exposure);
                }

                pixel_color = Vector4f(rgb, 1.0f);
            }

            image.setPixel(i, j, pixel_color);
        }
    }
    

    image.exportPNG(config.name.c_str());
    return 0;
}