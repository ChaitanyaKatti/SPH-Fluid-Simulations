#include <cuda_runtime.h>
#include <glm/glm.hpp>
#include <cmath>
#include <kernel.cuh>
#include <config.hpp>

// Ensure all vector operations are defined as __device__
__device__ inline glm::vec3 operator-(const glm::vec3 &v1, const glm::vec3 &v2) {
    return glm::vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

__device__ inline glm::vec3 operator*(const glm::vec3 &v, float s) {
    return glm::vec3(v.x * s, v.y * s, v.z * s);
}

__device__ inline glm::vec3 operator/(const glm::vec3 &v, float s) {
    return glm::vec3(v.x / s, v.y / s, v.z / s);
}

__device__ inline glm::vec3 operator+(const glm::vec3 &v1, const glm::vec3 &v2) {
    return glm::vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

__device__ inline float viscosityLaplacian(float sqrt_r) {
    if (sqrt_r < h) {
        return 45.0f / (M_PI * h6) * (h - sqrt_r);
    }
    return 0.0f;
}

__device__ inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r) {
    if (sqrt_r < h) {
        if (sqrt_r < 0.0001f) {
            return (float)(-45.0f / (M_PI * h6) * h2) * glm::normalize(r);
        }
        return (float)(-45.0f / (M_PI * h6) * glm::pow(h - sqrt_r, 2)) * r / (sqrt_r + DIVISON_EPSILON);
    }
    return glm::vec3(0.0f);
}

__device__ inline float poly6Kernel(float r2) {
    if (r2 < h2) {
        return (315.0f / (64.0f * M_PI * h9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

// Kernel function: Calculate density and pressure
__global__ void calculateDensityAndPressureKernel(glm::vec3 *positions, float *densities, float *pressures, int numParticles) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) return;

    densities[i] = 0.0f;
    for (int j = 0; j < numParticles; j++) {
        if (i == j) continue;
        glm::vec3 r = positions[j] - positions[i];
        float r2 = glm::dot(r, r);
        densities[i] += MASS * poly6Kernel(r2);
    }
    pressures[i] = k * fmaxf((densities[i] - RESTING_DENSITY), 0.0f);
}

// Kernel function: Apply forces (gravity, pressure, viscosity)
__global__ void applyForcesKernel(glm::vec3 *positions, glm::vec3 *velocities, glm::vec3 *forces, 
                                   float *pressures, float *densities, int numParticles) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) return;

    forces[i] = glm::vec3(0.0f, -MASS * g, 0.0f); // Gravity

    for (int j = 0; j < numParticles; j++) {
        if (i == j) continue;
        glm::vec3 r = positions[i] - positions[j];
        float sqrt_r = glm::length(r);
        forces[i] += -MASS * (pressures[i] + pressures[j]) / (2.0f * densities[j] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);  // Pressure term
        forces[i] += mu * MASS * (velocities[j] - velocities[i]) / (densities[j] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
    }
}

// Kernel function: Update particle positions and velocities
__global__ void updateParticlesKernel(glm::vec3 *positions, glm::vec3 *velocities, glm::vec3 *forces, 
                                      float *densities, int numParticles) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) return;

    // Update velocities and positions
    velocities[i] += dt * forces[i] / (densities[i] + DIVISON_EPSILON);
    if (glm::length(velocities[i]) > MAX_VELOCITY) {
        velocities[i] = glm::normalize(velocities[i]) * MAX_VELOCITY;
    }
    positions[i] += dt * velocities[i] + 0.5f * dt * dt * forces[i] / (densities[i] + DIVISON_EPSILON);

    // Apply boundary conditions (handled on CPU after kernel call)
}

// Kernel function: Resolve particle collisions
__global__ void resolveCollisionsKernel(glm::vec3 *positions, glm::vec3 *velocities, int numParticles) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) return;

    for (int j = i + 1; j < numParticles; j++) {
        float dist = glm::length(positions[i] - positions[j]);
        if (dist < 2.0f * Radius) {
            glm::vec3 normal = glm::normalize(positions[i] - positions[j]);
            positions[i] += normal * (Radius - 0.5f * dist);
            positions[j] -= normal * (Radius - 0.5f * dist);

            // Velocity restitution (elastic collision)
            velocities[i] -= glm::dot(velocities[i], normal) * normal * (1 + COEFF_RESTITUTION);
            velocities[j] -= glm::dot(velocities[j], normal) * normal * (1 + COEFF_RESTITUTION);
        }
    }
}
