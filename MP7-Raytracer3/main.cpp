#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <memory>
#include "Math.h"
#include "uselibpng.h"


// Forward declarations
class Material;
class Vector3;
class Vector4;

enum class CameraType {
        CLASSIC,
        FISHEYE,
        PANORAMA
    };

// Constants
constexpr float MIN_INTERSECTION_DISTANCE = 0.0001f;
constexpr float SHADOW_BIAS = 0.0001f;
constexpr int MAX_RAY_DEPTH = 5;

struct IntersectionInfo {
    float distance;
    Material* material;
    Vector3 surfaceNormal;
};


class Ray {
public:
    Ray(const Vector3& origin, const Vector3& direction, int depth = 0)
        : origin_(origin), direction_(direction.getNormalized()), depth_(depth) {}

    const Vector3& getOrigin() const { return origin_; }
    const Vector3& getDirection() const { return direction_; }
    Vector3 getPointAtDistance(float distance) const { return origin_.plus(direction_.times(distance)); }

private:
    Vector3 origin_;
    Vector3 direction_;
    int depth_;
};

class LightSource {
public:
    struct IlluminationInfo {
        Vector3 direction;
        Vector3 color;
        float distance;
    };

    virtual ~LightSource() = default;
    virtual IlluminationInfo calculateIllumination(const Vector3& point) const = 0;
};

class DirectionalLight : public LightSource {
public:
    DirectionalLight(const Vector3& direction, const Vector3& color)
        : direction_(direction.getNormalized()), color_(color) {}

    IlluminationInfo calculateIllumination(const Vector3& point) const override {
        return {direction_, color_, std::numeric_limits<float>::infinity()};
    }

private:
    Vector3 direction_;
    Vector3 color_;
};

class PointLight : public LightSource {
public:
    PointLight(const Vector3& position, const Vector3& color)
        : position_(position), color_(color) {}

    IlluminationInfo calculateIllumination(const Vector3& point) const override {
        Vector3 direction = position_.minus(point);
        float distance = direction.getLength();
        return {
            direction.getNormalized(),
            color_.times(1.0f / (distance * distance)),
            distance
        };
    }

private:
    Vector3 position_;
    Vector3 color_;
};

class Material {
public:
    Material(const Vector3& diffuseColor = Vector3(1, 1, 1))
        : diffuseColor_(diffuseColor) {}

    Vector3 calculateShading(const Ray& ray, const IntersectionInfo& intersection,
                           const Vector3& lightDir, const Vector3& lightColor) const {
        Vector3 normal = intersection.surfaceNormal;
        if (Vector3::dotProduct(normal, ray.getDirection()) > 0) {
            normal = normal.times(-1.0f);
        }
        float diffuseFactor = std::max(0.0f, Vector3::dotProduct(normal, lightDir));
        return Vector3::componentMultiply(lightColor, diffuseColor_).times(diffuseFactor);
    }

    const Vector3& getDiffuseColor() const { return diffuseColor_; }

private:
    Vector3 diffuseColor_;
};

class SceneObject {
public:
    explicit SceneObject(Material* material) : material_(material) {}
    virtual ~SceneObject() = default;
    virtual bool calculateIntersection(const Ray& ray, IntersectionInfo& intersection, 
                                     float minDistance) const = 0;

protected:
    Material* material_;
};

class Sphere : public SceneObject {
public:
    Sphere(float radius, const Vector3& center, Material* material)
        : SceneObject(material), radius_(radius), center_(center) {}

    bool calculateIntersection(const Ray& ray, IntersectionInfo& intersection,
                               float minDistance) const override {
        Vector3 oc = ray.getOrigin().minus(center_);
        float a = Vector3::dotProduct(ray.getDirection(), ray.getDirection());
        float b = 2.0f * Vector3::dotProduct(oc, ray.getDirection());
        float c = Vector3::dotProduct(oc, oc) - radius_ * radius_;
        float discriminant = b * b - 4 * a * c;

        if (discriminant < 0) {
            return false;
        }

        float sqrtDiscriminant = std::sqrt(discriminant);
        float t = (-b - sqrtDiscriminant) / (2.0f * a);

        if (t < minDistance) {
            t = (-b + sqrtDiscriminant) / (2.0f * a);
            if (t < minDistance) {
                return false;
            }
        }

        intersection.distance = t;
        intersection.material = material_;
        intersection.surfaceNormal = ray.getPointAtDistance(t).minus(center_).times(1.0f / radius_);

        return true;
    }

private:
    float radius_;
    Vector3 center_;
};

class Scene {
public:
    void addObject(SceneObject* object) { objects_.push_back(object); }
    void addLight(LightSource* light) { lights_.push_back(light); }
    
    const std::vector<LightSource*>& getLights() const { return lights_; }
    
    bool findNearestIntersection(const Ray& ray, IntersectionInfo& intersection,
                                  float minDistance) const {
        float nearestDistance = std::numeric_limits<float>::infinity();
        bool foundIntersection = false;

        for (const auto* object : objects_) {
            IntersectionInfo currentIntersection;
            if (object->calculateIntersection(ray, currentIntersection, minDistance)) {
                if (currentIntersection.distance < nearestDistance) {
                    nearestDistance = currentIntersection.distance;
                    intersection = currentIntersection;
                    foundIntersection = true;
                }
            }
        }

        return foundIntersection;
    }

private:
    std::vector<SceneObject*> objects_;
    std::vector<LightSource*> lights_;
};

// Replace the existing Camera class with this new version
class Camera {
public:
    Camera(const Vector3& position, const Vector3& lookDirection, const Vector3& upDirection,
       CameraType type, int width, int height)
    : position_(position), width_(width), height_(height), type_(type) 
    {
        // Don't normalize forward vector to support zoom through vector length
        forward_ = lookDirection;
        
        // Create orthonormal basis
        right_ = Vector3::crossProduct(forward_, upDirection).getNormalized();
        up_ = Vector3::crossProduct(right_, forward_).getNormalized();
    }

    Ray generateRay(float screenX, float screenY) const {
        switch (type_) {
            case CameraType::FISHEYE:
                return generateFisheyeRay(screenX, screenY);
            case CameraType::PANORAMA:
                return generatePanoramaRay(screenX, screenY);
            default:
                return generateClassicRay(screenX, screenY);
        }
    }

private:
    Vector3 position_;
    Vector3 forward_;
    Vector3 right_;
    Vector3 up_;
    int width_;
    int height_;
    CameraType type_;

    Ray generateClassicRay(float screenX, float screenY) const {
        float focalLength = 1.0f;  // Or compute based on desired field of view
        Vector3 direction = forward_.times(focalLength).plus(
            right_.times(screenX).plus(
                up_.times(screenY)
            )
    );
    
    return Ray(position_, direction.getNormalized());
    }

    Ray generateFisheyeRay(float screenX, float screenY) const {
        float r2 = screenX * screenX + screenY * screenY;
    
        if (r2 > 1.0f) {
            return Ray(position_, Vector3::ZERO);
        }

        // Use 1−sx²−sy² * forward as specified
        float scale = std::sqrt(1.0f - r2);
        Vector3 direction = forward_.times(scale).plus(
            right_.times(screenX).plus(
                up_.times(screenY)
            )
        );
        
        return Ray(position_, direction.getNormalized());
    }

    Ray generatePanoramaRay(float screenX, float screenY) const {
        // Map screen coordinates to spherical coordinates
        float theta = (screenX + 1.0f) * M_PI;  // longitude: [-π, π]
        float phi = (1.0f - screenY) * M_PI;    // latitude: [0, π]

        // Convert spherical to Cartesian coordinates
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        Vector3 direction = forward_.times(cosPhi * cosTheta).plus(
            right_.times(cosPhi * sinTheta).plus(
                up_.times(sinPhi)
            )
        );

        return Ray(position_, direction.getNormalized());
    }
};

class ImageRenderer {
public:
    ImageRenderer(int width, int height) 
        : width_(width), height_(height), image_(width, height) {}

    void setPixel(int x, int y, const Vector4& color) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            // Convert linear RGB to sRGB and then to 8-bit color
            pixel_t& pixel = image_[y][x];
            pixel.r = static_cast<uint8_t>(Math::clamp(Math::convertLinearToSRGB(color.x) * 255.0f, 0.0f, 255.0f));
            pixel.g = static_cast<uint8_t>(Math::clamp(Math::convertLinearToSRGB(color.y) * 255.0f, 0.0f, 255.0f));
            pixel.b = static_cast<uint8_t>(Math::clamp(Math::convertLinearToSRGB(color.z) * 255.0f, 0.0f, 255.0f));
            pixel.a = static_cast<uint8_t>(Math::clamp(color.w * 255.0f, 0.0f, 255.0f));
        }
    }

    void saveToFile(const char* filename) {
    image_.save(filename);
    }

private:
    int width_;
    int height_;
    Image image_;
};



class Plane : public SceneObject {
public:
    // Constructor takes A, B, C, D coefficients of the plane equation Ax + By + Cz + D = 0
    Plane(float A, float B, float C, float D, Material* material)
        : SceneObject(material), A_(A), B_(B), C_(C), D_(D) {
        // Calculate normalized normal vector from A, B, C coefficients
        float length = std::sqrt(A*A + B*B + C*C);
        if (length > MIN_INTERSECTION_DISTANCE) {
            normal_ = Vector3(A/length, B/length, C/length);
        } else {
            normal_ = Vector3(0, 1, 0);  // Default to up vector if degenerate
        }
    }

    bool calculateIntersection(const Ray& ray, IntersectionInfo& intersection,
                             float minDistance) const override {
        float denominator = A_ * ray.getDirection().x + 
                          B_ * ray.getDirection().y + 
                          C_ * ray.getDirection().z;
        
        // Ray is parallel to plane
        if (std::abs(denominator) < MIN_INTERSECTION_DISTANCE) {
            return false;
        }

        // Calculate intersection using plane equation
        float t = -(A_ * ray.getOrigin().x + 
                   B_ * ray.getOrigin().y + 
                   C_ * ray.getOrigin().z + D_) / denominator;
        
        if (t < minDistance) {
            return false;
        }

        intersection.distance = t;
        intersection.material = material_;
        intersection.surfaceNormal = denominator < 0 ? normal_ : normal_.times(-1.0f);
        
        return true;
    }

private:
    float A_, B_, C_, D_;  // Plane equation coefficients
    Vector3 normal_;       // Normalized normal vector (A,B,C)/sqrt(A²+B²+C²)
};



class Triangle : public SceneObject {
public:
    Triangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, Material* material)
        : SceneObject(material), v1_(v1), v2_(v2), v3_(v3) {
        // Calculate normal using cross product
        Vector3 edge1 = v2_.minus(v1_);
        Vector3 edge2 = v3_.minus(v1_);
        normal_ = Vector3::crossProduct(edge1, edge2).getNormalized();
    }

    bool calculateIntersection(const Ray& ray, IntersectionInfo& intersection,
                             float minDistance) const override {
        // Möller–Trumbore intersection algorithm
        Vector3 edge1 = v2_.minus(v1_);
        Vector3 edge2 = v3_.minus(v1_);
        
        Vector3 h = Vector3::crossProduct(ray.getDirection(), edge2);
        float a = Vector3::dotProduct(edge1, h);
        
        // Ray is parallel to triangle
        if (std::abs(a) < MIN_INTERSECTION_DISTANCE) {
            return false;
        }
        
        float f = 1.0f / a;
        Vector3 s = ray.getOrigin().minus(v1_);
        float u = f * Vector3::dotProduct(s, h);
        
        if (u < 0.0f || u > 1.0f) {
            return false;
        }
        
        Vector3 q = Vector3::crossProduct(s, edge1);
        float v = f * Vector3::dotProduct(ray.getDirection(), q);
        
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }
        
        float distance = f * Vector3::dotProduct(edge2, q);
        
        if (distance < minDistance) {
            return false;
        }
        
        intersection.distance = distance;
        intersection.material = material_;
        intersection.surfaceNormal = Vector3::dotProduct(normal_, ray.getDirection()) < 0 
                                   ? normal_ 
                                   : normal_.times(-1.0f);
        
        return true;
    }

private:
    Vector3 v1_, v2_, v3_;
    Vector3 normal_;
};



class SceneConfiguration {
public:
    struct Config {
        std::string outputFilename;
        int imageWidth;
        int imageHeight;
        Scene scene;
        Vector3 cameraPosition = Vector3::ZERO;
        Vector3 cameraForward = Vector3::FORWARD;
        Vector3 cameraUp = Vector3::UP;
        std::vector<std::unique_ptr<Material>> materials;
        bool useExposure = false;
        float exposureValue = 1.0f;
        std::vector<Vector3> vertices;
        CameraType cameraType = CameraType::CLASSIC;  // Updated to use the new enum

        Config() {
            materials.push_back(std::make_unique<Material>(Vector3(1, 1, 1)));
        }

        Camera createCamera() const {
            return Camera(cameraPosition, cameraForward, cameraUp, 
                        cameraType, imageWidth, imageHeight);
        }
    };

    int loadFromFile(const char* filename, Config& config) {
        std::ifstream inputFile(filename);
        std::string line;

        while (std::getline(inputFile, line)) {
            std::vector<std::string> command;
            parseCommand(line, command);
            if (command.empty()) continue;

            if (!processCommand(command, config)) {
                std::cerr << "Error processing command: " << command[0] << std::endl;
                return -1;
            }
        }
        return 0;
    }

private:
    void parseCommand(const std::string& line, std::vector<std::string>& command) {
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            command.push_back(token);
        }
    }

    bool processCommand(const std::vector<std::string>& command, Config& config) {
        const std::string& cmd = command[0];
        
        if (cmd == "png") return processImageSettings(command, config);
        if (cmd == "sphere") return processSphere(command, config);
        if (cmd == "color") return processMaterial(command, config);
        if (cmd == "sun") return processDirectionalLight(command, config);
        if (cmd == "bulb") return processPointLight(command, config);
        if (cmd == "expose") return processExposure(command, config);
        if (cmd == "eye") return processCameraPosition(command, config);
        if (cmd == "forward") return processCameraForward(command, config);
        if (cmd == "up") return processCameraUp(command, config);
        if (cmd == "plane") return processPlane(command, config);
        if (cmd == "xyz") return processVertex(command, config);
        if (cmd == "tri") return processTriangle(command, config);
        if (cmd == "fisheye") {
            config.cameraType = CameraType::FISHEYE;
            return true;
        }
        if (cmd == "panorama") {
            config.cameraType = CameraType::PANORAMA;
            return true;
        }
        
        return false;
    }

    bool processImageSettings(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        config.imageWidth = std::stoi(command[1]);
        config.imageHeight = std::stoi(command[2]);
        config.outputFilename = command[3];
        return true;
    }

    bool processSphere(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 5) return false;
        
        Vector3 center(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        );
        float radius = std::stof(command[4]);
        
        auto sphere = std::make_unique<Sphere>(
            radius,
            center,
            config.materials.back().get()
        );
        config.scene.addObject(sphere.release());
        return true;
    }

    bool processDirectionalLight(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        
        Vector3 direction(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        );
        
        auto light = std::make_unique<DirectionalLight>(
            direction,
            config.materials.back()->getDiffuseColor()
        );
        config.scene.addLight(light.release());
        return true;
    }

    bool processPointLight(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        
        Vector3 position(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        );
        
        auto light = std::make_unique<PointLight>(
            position,
            config.materials.back()->getDiffuseColor()
        );
        config.scene.addLight(light.release());
        return true;
    }

    // Add missing process methods to SceneConfiguration class
    bool processMaterial(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        
        Vector3 color(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        );
        
        config.materials.push_back(std::make_unique<Material>(color));
        return true;
    }

    bool processExposure(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 2) return false;
        config.useExposure = true;
        config.exposureValue = std::stof(command[1]);
        return true;
    }

    bool processCameraPosition(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        config.cameraPosition = Vector3(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        );
        return true;
    }

    bool processCameraForward(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        config.cameraForward = Vector3(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        ).getNormalized();
        return true;
    }

    bool processCameraUp(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        config.cameraUp = Vector3(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        ).getNormalized();
        return true;
    }

    // Update the processPlane method in SceneConfiguration class:
    bool processPlane(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 5) return false;  // Expect: plane A B C D
        
        float A = std::stof(command[1]);
        float B = std::stof(command[2]);
        float C = std::stof(command[3]);
        float D = std::stof(command[4]);
        
        auto plane = std::make_unique<Plane>(A, B, C, D, config.materials.back().get());
        config.scene.addObject(plane.release());
        return true;
    }

    bool processVertex(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;

        config.vertices.push_back(Vector3(
            std::stof(command[1]),
            std::stof(command[2]),
            std::stof(command[3])
        ));
        return true;
    }

    bool processTriangle(const std::vector<std::string>& command, Config& config) {
        if (command.size() != 4) return false;
        // Convert 1-based indices to 0-based for internal use
        int v1 = std::stoi(command[1]) - 1;  // Convert 1-based to 0-based
        int v2 = std::stoi(command[2]) - 1;  // Convert 1-based to 0-based
        int v3 = std::stoi(command[3]) - 1;  // Convert 1-based to 0-based
        
        // Check if indices are valid (including negative indices)
        int size = static_cast<int>(config.vertices.size());
        
        // Handle negative indices (counting from back)
        if (v1 < 0) v1 = size + v1 + 1;
        if (v2 < 0) v2 = size + v2 + 1;
        if (v3 < 0) v3 = size + v3 + 1;
        
        // Verify indices are within bounds
        if (v1 < 0 || v1 >= size || 
            v2 < 0 || v2 >= size || 
            v3 < 0 || v3 >= size) {
            return false;
        }
        
        auto triangle = std::make_unique<Triangle>(
            config.vertices[v1],
            config.vertices[v2],
            config.vertices[v3],
            config.materials.back().get()
        );
        config.scene.addObject(triangle.release());
        return true;
    }
};





class RayTracer {
public:
    struct TraceResult {
        Vector3 color;
        bool hitSomething;
    };

    static TraceResult traceRay(const Ray& ray, const Scene& scene) {
        IntersectionInfo intersection;
        Vector3 finalColor(0, 0, 0);
        bool hit = scene.findNearestIntersection(ray, intersection, MIN_INTERSECTION_DISTANCE);

        if (hit) {
            const auto& lights = scene.getLights();
            
            for (const auto* light : lights) {
                auto illumination = light->calculateIllumination(
                    ray.getPointAtDistance(intersection.distance)
                );

                // Check for shadows
                Ray shadowRay(
                    ray.getPointAtDistance(intersection.distance),
                    illumination.direction
                );
                IntersectionInfo shadowIntersection;

                bool inShadow = scene.findNearestIntersection(
                    shadowRay,
                    shadowIntersection,
                    SHADOW_BIAS
                ) && shadowIntersection.distance < illumination.distance;

                if (!inShadow) {
                    finalColor = finalColor.plus(
                        intersection.material->calculateShading(
                            ray,
                            intersection,
                            illumination.direction,
                            illumination.color
                        )
                    );
                }
            }
        }

        return {finalColor, hit};
    }
};




int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return -1;
    }

    SceneConfiguration::Config config;
    SceneConfiguration configLoader;
    
    if (configLoader.loadFromFile(argv[1], config) != 0) {
        std::cerr << "Failed to load configuration file" << std::endl;
        return -1;
    }

    ImageRenderer renderer(config.imageWidth, config.imageHeight);
    Camera camera = config.createCamera();
    const Scene& scene = config.scene;

    for (int x = 0; x < config.imageWidth; ++x) {
        for (int y = 0; y < config.imageHeight; ++y) {
            float aspectRatio = std::max(config.imageWidth, config.imageHeight);
            float screenX = (2.0f * x - config.imageWidth) / aspectRatio;
            float screenY = (config.imageHeight - 2.0f * y) / aspectRatio;

            Ray ray = camera.generateRay(screenX, screenY);
            auto traceResult = RayTracer::traceRay(ray, scene);
            Vector3 pixelColor = traceResult.color;  // Use the color from traceResult
            
            if (config.useExposure) {
                pixelColor = Vector3(
                    Math::calculateExposure(pixelColor.x, config.exposureValue),
                    Math::calculateExposure(pixelColor.y, config.exposureValue),
                    Math::calculateExposure(pixelColor.z, config.exposureValue)
                );
            }

            // Set alpha to 0 for background (no hit), 1 for objects
            float alpha = traceResult.hitSomething ? 1.0f : 0.0f;
            renderer.setPixel(x, y, Vector4(pixelColor, alpha));
        }
    }

    renderer.saveToFile(config.outputFilename.c_str());
    return 0;
}