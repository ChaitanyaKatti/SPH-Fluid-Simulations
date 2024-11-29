#include <cuda_runtime.h>
#include <vec3.hpp>
#include <config.hpp>

// Kernel functions for SPH simulation
__device__ inline float poly6Kernel(float r2)
{ // Poly6 kernel
    return (r2 < h2) ? (315.0f / (64.0f * M_PI * h9)) * powf(h2 - r2, 3) : 0.0f;
}
__device__ inline Vec3 spikyGradient(Vec3 r, float sqrt_r)
{ // Gradient of Spiky power 2 kernel
    if (sqrt_r < h1)
    {
        return (float)(-15.0f / (M_PI * h5) * (h1 - sqrt_r)) * r * (1.0f / (sqrt_r));
    }
    return Vec3(0.0f);
}
__device__ inline Vec3 spikyGradientNear(Vec3 r, float sqrt_r)
{ // Gradient of Spiky power 3 kernel
    if (sqrt_r < h1)
    {
        return (float)(-45.0f / (M_PI * h6) * powf(h1 - sqrt_r, 2)) * r * (1.0f / (sqrt_r));
    }
    return Vec3(0.0f);
}
__device__ inline float viscosityLaplacian(float sqrt_r)
{ // Laplacian of Viscosity kernel
    return (sqrt_r < h1) ? 45.0f / (M_PI * h6) * (h1 - sqrt_r) : 0.0f;
}