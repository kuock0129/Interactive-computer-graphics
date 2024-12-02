#ifndef MATH_H
#define MATH_H

#include <cmath>

namespace Math {
    constexpr float EPSILON = 0.0001f;
    constexpr float PI = 3.14159265359f;
    
    float clamp(float value, float minValue, float maxValue);
    float convertLinearToSRGB(float linearValue);
    float calculateExposure(float value, float exposure);
}

class Vector3 {
public:
    static const Vector3 ZERO;
    static const Vector3 FORWARD;
    static const Vector3 RIGHT;
    static const Vector3 UP;

    Vector3();
    Vector3(float xVal, float yVal, float zVal);

    Vector3& add(const Vector3& other);
    Vector3& subtract(const Vector3& other);
    Vector3& multiplyScalar(float scalar);
    Vector3& divideScalar(float scalar);
    
    Vector3 plus(const Vector3& other) const;
    Vector3 minus(const Vector3& other) const;
    Vector3 times(float scalar) const;
    Vector3 dividedBy(float scalar) const;

    static Vector3 componentMultiply(const Vector3& a, const Vector3& b);
    static Vector3 crossProduct(const Vector3& a, const Vector3& b);
    static float dotProduct(const Vector3& a, const Vector3& b);

    float getLength() const;
    float getLengthSquared() const;
    Vector3 getNormalized() const;
    void normalize();
    void clampValues(float minVal, float maxVal);

    float x;
    float y;
    float z;
};

class Vector4 {
public:
    Vector4();
    Vector4(float xVal, float yVal, float zVal, float wVal);
    Vector4(const Vector3& vec3, float wVal);

    Vector4& add(const Vector4& other);
    Vector4& multiplyScalar(float scalar);
    
    Vector4 plus(const Vector4& other) const;
    Vector4 times(float scalar) const;
    
    Vector3 toVector3() const;

    float x;
    float y;
    float z;
    float w;
};

#endif // MATH_H