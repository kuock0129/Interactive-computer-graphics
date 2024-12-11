#ifndef GRAPHICS_MATH_H
#define GRAPHICS_MATH_H

#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

namespace Math {
    // Type definitions
    using Vec2d = std::pair<double, double>;
    using Vec3d = std::tuple<double, double, double>;
    using Vec4d = std::tuple<double, double, double, double>;

    // Constants
    namespace Constants {
        constexpr double PI = 3.14159265358979323846;
        constexpr double EPSILON = 1e-10;
        constexpr double DEG_TO_RAD = PI / 180.0;
        constexpr double RAD_TO_DEG = 180.0 / PI;
    }

    // Basic math operations
    template<typename T>
    T clamp(T value, T min, T max) {
        return std::max(min, std::min(max, value));
    }


    inline double takeDecimal(double num) {
        double intpart;
        num = std::modf(num, &intpart);
        return (num < 0) ? num + 1.0 : num;
    }

    inline double lerp(double a, double b, double t) {
        return a + t * (b - a);
    }

    // Vector operations
    class Vector {
    public:
        static double dot(const Vec3d& v1, const Vec3d& v2) {
            const auto& [x1, y1, z1] = v1;
            const auto& [x2, y2, z2] = v2;
            return x1 * x2 + y1 * y2 + z1 * z2;
        }

        static Vec3d cross(const Vec3d& v1, const Vec3d& v2) {
            const auto& [x1, y1, z1] = v1;
            const auto& [x2, y2, z2] = v2;
            return {
                y1 * z2 - z1 * y2,
                z1 * x2 - x1 * z2,
                x1 * y2 - y1 * x2
            };
        }

        static double length(const Vec3d& v) {
            const auto& [x, y, z] = v;
            return std::sqrt(x * x + y * y + z * z);
        }

        static Vec3d normalize(const Vec3d& v) {
            double len = length(v);
            if (len < Constants::EPSILON) return {0, 0, 0};
            const auto& [x, y, z] = v;
            return {x / len, y / len, z / len};
        }
    };

    // Matrix operations
    class Matrix {
    public:
        static std::vector<double> multiply4x4(
            const std::vector<double>& mat1,
            const std::vector<double>& mat2) 
        {
            std::vector<double> result(16, 0.0);
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    for (int k = 0; k < 4; ++k) {
                        result[i * 4 + j] += mat1[i * 4 + k] * mat2[k * 4 + j];
                    }
                }
            }
            return result;
        }

        static Vec4d multiplyMat4Vec4(
            const std::vector<double>& mat,
            const Vec4d& vec) 
        {
            const auto& [x, y, z, w] = vec;
            return {
                mat[0] * x + mat[1] * y + mat[2] * z + mat[3] * w,
                mat[4] * x + mat[5] * y + mat[6] * z + mat[7] * w,
                mat[8] * x + mat[9] * y + mat[10] * z + mat[11] * w,
                mat[12] * x + mat[13] * y + mat[14] * z + mat[15] * w
            };
        }

        static std::vector<double> createIdentity4x4() {
            std::vector<double> mat(16, 0.0);
            mat[0] = mat[5] = mat[10] = mat[15] = 1.0;
            return mat;
        }

        static std::vector<double> createTranslation(double x, double y, double z) {
            auto mat = createIdentity4x4();
            mat[3] = x;
            mat[7] = y;
            mat[11] = z;
            return mat;
        }

        static std::vector<double> createScale(double x, double y, double z) {
            auto mat = createIdentity4x4();
            mat[0] = x;
            mat[5] = y;
            mat[10] = z;
            return mat;
        }

        static std::vector<double> createRotationX(double angle) {
            auto mat = createIdentity4x4();
            double c = std::cos(angle * Constants::DEG_TO_RAD);
            double s = std::sin(angle * Constants::DEG_TO_RAD);
            mat[5] = c;
            mat[6] = -s;
            mat[9] = s;
            mat[10] = c;
            return mat;
        }

        static std::vector<double> createRotationY(double angle) {
            auto mat = createIdentity4x4();
            double c = std::cos(angle * Constants::DEG_TO_RAD);
            double s = std::sin(angle * Constants::DEG_TO_RAD);
            mat[0] = c;
            mat[2] = s;
            mat[8] = -s;
            mat[10] = c;
            return mat;
        }

        static std::vector<double> createRotationZ(double angle) {
            auto mat = createIdentity4x4();
            double c = std::cos(angle * Constants::DEG_TO_RAD);
            double s = std::sin(angle * Constants::DEG_TO_RAD);
            mat[0] = c;
            mat[1] = -s;
            mat[4] = s;
            mat[5] = c;
            return mat;
        }
    };

    // Color operations
    class Color {
    public:
        static unsigned char linear2char(double color) {
            return static_cast<unsigned char>(clamp(color * 255.0, 0.0, 255.0));
        }

        static unsigned char sRGB2char(double color) {
            if (color <= 0.0031308) {
                color *= 12.92;
            } else {
                color = 1.055 * std::pow(color, 1.0 / 2.4) - 0.055;
            }
            return linear2char(color);
        }

        static double char2linear(unsigned char color) {
            return static_cast<double>(color) / 255.0;
        }

        static double char2sRGB(unsigned char color) {
            double c = char2linear(color);
            if (c <= 0.04045) {
                return c / 12.92;
            }
            return std::pow((c + 0.055) / 1.055, 2.4);
        }
    };

    // String parsing utilities
    class Parser {
    public:
        static size_t getToken(const std::string& str, std::string& tok, size_t pos = 0) {
            const std::string del = " \t";
            size_t begin = str.find_first_not_of(del, pos);
            if (begin == std::string::npos) { 
                tok = ""; 
                return begin; 
            }
            size_t end = str.find_first_of(del, begin);
            tok = str.substr(begin, end - begin);
            return end;
        }

        static bool parseInteger(const std::string& str, int& num) {
            try {
                num = std::stoi(str);
                return true;
            } catch (...) {
                return false;
            }
        }

        static bool parseDouble(const std::string& str, double& num) {
            try {
                num = std::stod(str);
                return true;
            } catch (...) {
                return false;
            }
        }
    };
}

#endif // GRAPHICS_MATH_H