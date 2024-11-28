#include <glm/glm.hpp>
#include <cuda_runtime.h>

// Vector operations are defined as __device__

// Assignment operators
__device__ inline glm::vec3 operator+=(glm::vec3 &v1, const glm::vec3 &v2)
{
    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
    return v1;
}
__device__ inline glm::vec3 operator-=(glm::vec3 &v1, const glm::vec3 &v2)
{
    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;
    return v1;
}
__device__ inline glm::vec3 operator*=(glm::vec3 &v, float s)
{
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return v;
}

// Addtion
__device__ inline glm::vec3 operator+(const glm::vec3 &v1, const glm::vec3 &v2)
{
    return glm::vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}
__device__ inline glm::vec3 operator+(const glm::vec3 &v, float s)
{
    return glm::vec3(v.x + s, v.y + s, v.z + s);
}
__device__ inline glm::vec3 operator+(float s, const glm::vec3 &v)
{
    return glm::vec3(v.x + s, v.y + s, v.z + s);
}

// Subtraction
__device__ inline glm::vec3 operator-(const glm::vec3 &v1, const glm::vec3 &v2)
{
    return glm::vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}
__device__ inline glm::vec3 operator-(const glm::vec3 &v, float s)
{
    return glm::vec3(v.x - s, v.y - s, v.z - s);
}
__device__ inline glm::vec3 operator-(float s, const glm::vec3 &v)
{
    return glm::vec3(v.x - s, v.y - s, v.z - s);
}

// Multiplication
__device__ inline glm::vec3 operator*(const glm::vec3 &v, float s)
{
    return glm::vec3(v.x * s, v.y * s, v.z * s);
}
__device__ inline glm::vec3 operator*(float s, const glm::vec3 &v)
{
    return glm::vec3(v.x * s, v.y * s, v.z * s);
}

// Division
__device__ inline glm::vec3 operator/(const glm::vec3 &v, float s)
{
    return glm::vec3(v.x / s, v.y / s, v.z / s);
}
