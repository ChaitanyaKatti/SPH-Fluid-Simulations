#pragma once

#include <cmath>
#ifdef __CUDACC__
// Custom vector class to replace GLM vec3
struct Vec3 {
    float x, y, z;

    // Constructors
    __host__ __device__ Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    __host__ __device__ Vec3(float val) : x(val), y(val), z(val) {}
    __host__ __device__ Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Operator overloads
    __host__ __device__ Vec3 operator+(const Vec3& v) const {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }

    __host__ __device__ Vec3 operator-(const Vec3& v) const {
        return Vec3(x - v.x, y - v.y, z - v.z);
    }

    __host__ __device__ Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
    __host__ __device__ Vec3 operator/(float scalar) const {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }

    __host__ __device__ Vec3& operator+=(const Vec3& v) {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }

    __host__ __device__ Vec3& operator-=(const Vec3& v) {
        x -= v.x; y -= v.y; z -= v.z;
        return *this;
    }

    __host__ __device__ Vec3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    // Dot product
    __host__ __device__ float dot(const Vec3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    // Cross product
    __host__ __device__ Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }

    // Length/magnitude
    __host__ __device__ float length() const {
        return sqrtf(x*x + y*y + z*z);
    }

    // Normalize
    __host__ __device__ Vec3 normalize() const {
        float len = length();
        return (len > 0.0f) ? Vec3(x/len, y/len, z/len) : Vec3(0.0f);
    }
};
// Additional math utility functions
__host__ __device__ inline Vec3 operator*(float scalar, const Vec3& v) {
    return v * scalar;
}

#else
struct Vec3 {
    float x, y, z;
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float val) : x(val), y(val), z(val) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};
#endif