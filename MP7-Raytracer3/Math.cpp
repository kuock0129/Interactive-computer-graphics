#include "Math.h"
#include <algorithm>

namespace Math {
    float clamp(float value, float minValue, float maxValue) {
        return std::max(minValue, std::min(maxValue, value));
    }
    
    float convertLinearToSRGB(float linearValue) {
        if (linearValue <= 0.0031308f) {
            return 12.92f * linearValue;
        }
        return 1.055f * std::pow(linearValue, 1.0f/2.4f) - 0.055f;
    }

    float calculateExposure(float value, float exposure) {
        return 1.0f - std::exp(-exposure * value);
    }
}

// Vector3 static constant definitions
const Vector3 Vector3::ZERO(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::FORWARD(0.0f, 0.0f, -1.0f);
const Vector3 Vector3::RIGHT(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::UP(0.0f, 1.0f, 0.0f);

// Vector3 implementation
Vector3::Vector3() : x(0.0f), y(0.0f), z(0.0f) {}

Vector3::Vector3(float xVal, float yVal, float zVal) 
    : x(xVal), y(yVal), z(zVal) {}

Vector3& Vector3::add(const Vector3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3& Vector3::subtract(const Vector3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector3& Vector3::multiplyScalar(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3& Vector3::divideScalar(float scalar) {
    float reciprocal = 1.0f / scalar;
    return multiplyScalar(reciprocal);
}

Vector3 Vector3::plus(const Vector3& other) const {
    return Vector3(*this).add(other);
}

Vector3 Vector3::minus(const Vector3& other) const {
    return Vector3(*this).subtract(other);
}

Vector3 Vector3::times(float scalar) const {
    return Vector3(*this).multiplyScalar(scalar);
}

Vector3 Vector3::dividedBy(float scalar) const {
    return Vector3(*this).divideScalar(scalar);
}

Vector3 Vector3::componentMultiply(const Vector3& a, const Vector3& b) {
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

Vector3 Vector3::crossProduct(const Vector3& a, const Vector3& b) {
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float Vector3::dotProduct(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Vector3::getLength() const {
    return std::sqrt(getLengthSquared());
}

float Vector3::getLengthSquared() const {
    return x * x + y * y + z * z;
}

Vector3 Vector3::getNormalized() const {
    float length = getLength();
    if (length > Math::EPSILON) {
        return dividedBy(length);
    }
    return Vector3::ZERO;
}

void Vector3::normalize() {
    float length = getLength();
    if (length > Math::EPSILON) {
        divideScalar(length);
    }
}

void Vector3::clampValues(float minVal, float maxVal) {
    x = Math::clamp(x, minVal, maxVal);
    y = Math::clamp(y, minVal, maxVal);
    z = Math::clamp(z, minVal, maxVal);
}

// Vector4 implementation
Vector4::Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

Vector4::Vector4(float xVal, float yVal, float zVal, float wVal)
    : x(xVal), y(yVal), z(zVal), w(wVal) {}

Vector4::Vector4(const Vector3& vec3, float wVal)
    : x(vec3.x), y(vec3.y), z(vec3.z), w(wVal) {}

Vector4& Vector4::add(const Vector4& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

Vector4& Vector4::multiplyScalar(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

Vector4 Vector4::plus(const Vector4& other) const {
    return Vector4(*this).add(other);
}

Vector4 Vector4::times(float scalar) const {
    return Vector4(*this).multiplyScalar(scalar);
}

Vector3 Vector4::toVector3() const {
    return Vector3(x, y, z);
}