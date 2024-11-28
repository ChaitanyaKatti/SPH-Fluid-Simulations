#include <glad/glad.h>
#include <config.hpp>
#include <bits/stdc++.h>
#include <cuda_runtime.h>
#include <particles.cuh>

#include <cuda_runtime.h>
#include <glm/glm.hpp>
#include <cmath>
#include <config.hpp>
#include <time.h>

// Ensure all vector operations are defined as __device__
__device__ inline glm::vec3 operator-(const glm::vec3 &v1, const glm::vec3 &v2)
{
    return glm::vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

__device__ inline glm::vec3 operator*(const glm::vec3 &v, float s)
{
    return glm::vec3(v.x * s, v.y * s, v.z * s);
}

__device__ inline glm::vec3 operator/(const glm::vec3 &v, float s)
{
    return glm::vec3(v.x / s, v.y / s, v.z / s);
}

__device__ inline glm::vec3 operator+(const glm::vec3 &v1, const glm::vec3 &v2)
{
    return glm::vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

__device__ inline float viscosityLaplacian(float sqrt_r)
{
    if (sqrt_r < h1)
    {
        return 45.0f / (M_PI * h6) * (h1 - sqrt_r);
    }
    return 0.0f;
}

__device__ inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r)
{
    if (sqrt_r < h1)
    {
        if (sqrt_r < 0.0001f)
        {
            return (float)(-45.0f / (M_PI * h6) * h2) * glm::normalize(r);
        }
        return (float)(-45.0f / (M_PI * h6) * glm::pow(h1 - sqrt_r, 2)) * r / (sqrt_r + DIVISON_EPSILON);
    }
    return glm::vec3(0.0f);
}

__device__ inline float poly6Kernel(float r2)
{
    if (r2 < h2)
    {
        return (315.0f / (64.0f * M_PI * h9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

// Kernel function: Calculate density and pressure
__global__ void calculateDensityAndPressureKernel(glm::vec3 *positions, float *densities, float *pressures, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    densities[i] = 0.0f;
    for (int j = 0; j < numParticles; j++)
    {
        if (i == j)
            continue;
        glm::vec3 r = positions[j] - positions[i];
        float r2 = glm::dot(r, r);
        densities[i] += MASS * poly6Kernel(r2);
    }
    pressures[i] = BULK_MODULUS * fmaxf((densities[i] - RESTING_DENSITY), 0.0f);
}

// Kernel function: Apply forces (gravity, pressure, viscosity)
__global__ void applyForcesKernel(glm::vec3 *positions, glm::vec3 *velocities, glm::vec3 *forces,
                                  float *pressures, float *densities, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    forces[i] = glm::vec3(0.0f, -MASS * gravity, 0.0f); // Gravity

    for (int j = 0; j < numParticles; j++)
    {
        if (i == j)
            continue;
        glm::vec3 r = positions[i] - positions[j];
        float sqrt_r = glm::length(r);
        forces[i] += -MASS * (pressures[i] + pressures[j]) / (2.0f * densities[j] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);  // Pressure term
        forces[i] += mu * MASS * (velocities[j] - velocities[i]) / (densities[j] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
    }
}

// Kernel function: Update particle positions and velocities
__global__ void updateParticlesKernel(glm::vec3 *positions, glm::vec3 *colors, glm::vec3 *velocities, glm::vec3 *forces,
                                      float *densities, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    // Update velocities and positions
    velocities[i] += dt * forces[i] / (densities[i] + DIVISON_EPSILON);
    if (glm::length(velocities[i]) > MAX_VELOCITY)
    {
        velocities[i] = glm::normalize(velocities[i]) * MAX_VELOCITY;
    }
    positions[i] += dt * velocities[i] + 0.5f * dt * dt * forces[i] / (densities[i] + DIVISON_EPSILON);

    // Boundary conditions
    if (positions[i].x < 0.0f)
    {
        positions[i].x = 0.0f + EPSILON;
        velocities[i].x *= -COEFF_RESTITUTION;
    }
    if (positions[i].x > 10.0f)
    {
        positions[i].x = 10.0f - EPSILON;
        velocities[i].x *= -COEFF_RESTITUTION;
    }
    if (positions[i].y < 0.0f)
    {
        positions[i].y = 0.0f + EPSILON;
        velocities[i].y *= -COEFF_RESTITUTION;
    }
    if (positions[i].y > 10.0f)
    {
        positions[i].y = 10.0f - EPSILON;
        velocities[i].y *= -COEFF_RESTITUTION;
    }
    if (positions[i].z < 0.0f)
    {
        positions[i].z = 0.0f + EPSILON;
        velocities[i].z *= -COEFF_RESTITUTION;
    }
    if (positions[i].z > 5.0f)
    {
        positions[i].z = 5.0f - EPSILON;
        velocities[i].z *= -COEFF_RESTITUTION;
    }

    // Update colors based on particle density
    float density = densities[i];
    colors[i] = glm::vec3(1.0f, 0.0f, 0.0f) * (1.0f - fminf(fmaxf(density / RESTING_DENSITY, 0.0f), 1.0f)) +
                glm::vec3(0.0f, 1.0f, 0.0f) * fminf(fmaxf(density / RESTING_DENSITY, 0.0f), 1.0f);
}

// Kernel function: Resolve particle collisions
__global__ void resolveCollisionsKernel(glm::vec3 *positions, glm::vec3 *velocities, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    for (int j = i + 1; j < numParticles; j++)
    {
        float dist = glm::length(positions[i] - positions[j]);
        if (dist < 2.0f * Radius)
        {
            glm::vec3 normal = glm::normalize(positions[i] - positions[j]);
            positions[i] += normal * (Radius - 0.5f * dist);
            positions[j] -= normal * (Radius - 0.5f * dist);

            // Velocity restitution (elastic collision)
            velocities[i] -= glm::dot(velocities[i], normal) * normal * (1 + COEFF_RESTITUTION);
            velocities[j] -= glm::dot(velocities[j], normal) * normal * (1 + COEFF_RESTITUTION);
        }
    }
}

__host__ Particles::Particles(Shader *const shader) : shader(shader)
{
    // CUDA memory pointers for particle attributes
    glm::vec3 *d_positions, *d_velocities, *d_forces;
    float *d_densities, *d_pressures;

    // Initialize arrays for SPH
    this->h_positions = new glm::vec3[NUM_INS];
    this->h_colors = new glm::vec3[NUM_INS];
    std::cout << "Volume: " << pow(MASS * NUM_INS / RESTING_DENSITY, 1.0 / 3.0) << std::endl;
    genUniformVec3Array(h_positions, NUM_INS_DIM, 5.0f);

    for (int i = 0; i < NUM_INS; i++)
    {
        h_colors[i] = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // CUDA memory allocation for particle data
    cudaMalloc((void **)&d_positions, NUM_INS * sizeof(glm::vec3));
    cudaMalloc((void **)&d_velocities, NUM_INS * sizeof(glm::vec3));
    cudaMalloc((void **)&d_forces, NUM_INS * sizeof(glm::vec3));
    cudaMalloc((void **)&d_densities, NUM_INS * sizeof(float));
    cudaMalloc((void **)&d_pressures, NUM_INS * sizeof(float));

    cudaMemcpy(d_positions, h_positions, NUM_INS * sizeof(glm::vec3), cudaMemcpyHostToDevice);

    // Rendering
    setupVAO();
}

__host__ void Particles::setupVAO()
{
    // VAO : Vertex Array Object
    glGenVertexArrays(1, &VAO);
    // VBO : Vertex Buffer Object
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3) * 2, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), h_positions);
    glBufferSubData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3), NUM_INS * sizeof(glm::vec3), h_colors);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)(NUM_INS * sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

__host__ void Particles::updateGPU()
{
    // Launch kernels to compute densities, pressures, forces, update particles and resolve collisions
    int blockSize = 256; // 256 threads per block
    int numBlocks = (NUM_INS + blockSize - 1) / blockSize;

    // Calculate density and pressure in parallel
    // calculateDensityAndPressureKernel<<<numBlocks, blockSize>>>(d_positions, d_densities, d_pressures, NUM_INS);
    // cudaDeviceSynchronize();
    
    // cudaError_t error = cudaGetLastError();
    // if (error != cudaSuccess)
    // {
    //     std::cerr << "CUDA error in updateGPU: " << cudaGetErrorString(error) << std::endl;
    // }
    // exit(1);

    // Apply forces (pressure, viscosity, gravity) in parallel
    // applyForcesKernel<<<numBlocks, blockSize>>>(d_positions, d_velocities, d_forces, d_pressures, d_densities, NUM_INS);
    // cudaDeviceSynchronize();

    // // Update particles (positions, velocities)
    // updateParticlesKernel<<<numBlocks, blockSize>>>(d_positions, d_colors, d_velocities, d_forces, d_densities, NUM_INS);
    // cudaDeviceSynchronize();

    // // Resolve collisions between particles
    // resolveCollisionsKernel<<<numBlocks, blockSize>>>(d_positions, d_velocities, NUM_INS);
    // cudaDeviceSynchronize();

    // Copy the updated particle positions back to host
    cudaMemcpy(h_positions, d_positions, NUM_INS * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_colors, d_colors, NUM_INS * sizeof(glm::vec3), cudaMemcpyDeviceToHost);


    // Update VBO for rendering
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), h_positions);
    glBufferSubData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3), NUM_INS * sizeof(glm::vec3), h_colors);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

__host__ void Particles::Draw()
{
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", Radius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, NUM_INS);
    glBindVertexArray(0);
}

__host__ void Particles::setPositions(glm::vec3 *h_positions)
{
    this->h_positions = h_positions;
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), h_positions);
    glBindVertexArray(0);

    for (int i = 0; i < NUM_INS; i++)
    {
        h_colors[i] = glm::vec3(1.0f);
    }

    // Copy data to CUDA memory
    cudaMemcpy(d_positions, h_positions, NUM_INS * sizeof(glm::vec3), cudaMemcpyHostToDevice);
}

__host__ void Particles::reset()
{
    genUniformVec3Array(h_positions, NUM_INS_DIM, 5.0f);
    setPositions(h_positions);
}

// Utility function: Generate a uniform array of glm::vec3
__host__ void Particles::genUniformVec3Array(glm::vec3 *arr, int n, float scale)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < n; k++)
            {
                if (n == 1)
                {
                    arr[i * n * n + j * n + k] = glm::vec3(0.0f);
                    continue;
                }
                float factor = scale / n - 1;
                arr[i * n * n + j * n + k] = glm::vec3(i * factor, j * factor, k * factor);
            }
        }
    }
}

Particles::~Particles()
{
    // Clean up CUDA memory
    cudaFree(d_positions);
    cudaFree(d_colors);
    cudaFree(d_velocities);
    cudaFree(d_forces);
    cudaFree(d_densities);
    cudaFree(d_pressures);

    // Clean up host memory
    delete[] h_positions;
    delete[] h_colors;
}
