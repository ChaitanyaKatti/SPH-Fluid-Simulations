// kernel.cuh
#ifndef KERNEL_CUH
#define KERNEL_CUH

#include <glm/glm.hpp>

// CUDA kernel for calculating densities and pressures
__global__ void calculateDensityAndPressureKernel(glm::vec3 *positions, float *densities, float *pressures, int numParticles);

// CUDA kernel for applying forces (pressure, viscosity, gravity)
__global__ void applyForcesKernel(glm::vec3 *positions, glm::vec3 *velocities, glm::vec3 *forces, 
                                   float *pressures, float *densities, int numParticles);

// CUDA kernel for updating particle positions and velocities
__global__ void updateParticlesKernel(glm::vec3 *positions, glm::vec3 *velocities, glm::vec3 *forces, 
                                      float *densities, int numParticles);

// CUDA kernel for resolving collisions between particles
__global__ void resolveCollisionsKernel(glm::vec3 *positions, glm::vec3 *velocities, int numParticles);

#endif // KERNEL_CUH
