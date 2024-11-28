#include <glad/glad.h>
#include <particles.hpp>
#include <config.hpp>
#include <bits/stdc++.h>
#include <cuda_runtime.h>
#include <kernel.cuh>

inline glm::vec3 getRandVec3()
{
    return glm::vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
}

void genUniformVec3Array(glm::vec3 *arr, int n, float scale = 1.0f)
{
    if (n <= 0)
    {
        return;
    }
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < n; k++)
            {
                arr[i * n * n + j * n + k] = glm::vec3(i, j, k) * (scale / (n - 1)) + 0.01f * getRandVec3();
            }
        }
    }
}

inline float poly6Kernel(float r2)
{
    if (r2 < h2)
    {
        return (315.0f / (64.0f * M_PI * h9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r)
{
    if (sqrt_r < h)
    {
        if (sqrt_r < 0.0001f)
        {
            return (float)(-45.0f / (M_PI * h6) * h2) * glm::normalize(getRandVec3());
        }
        return (float)(-45.0f / (M_PI * h6) * glm::pow(h - sqrt_r, 2)) * r / (sqrt_r + DIVISON_EPSILON);
    }
    return glm::vec3(0.0f);
}

inline float viscosityLaplacian(float sqrt_r)
{
    if (sqrt_r < h)
    {
        return 45.0f / (M_PI * h6) * (h - sqrt_r);
    }
    return 0.0f;
}

// CUDA memory pointers for particle attributes
glm::vec3 *d_positions, *d_velocities, *d_forces;
float *d_densities, *d_pressures;

Particles::Particles(Shader *const shader) : shader(shader)
{
    // Initialize arrays for SPH
    this->positions = new glm::vec3[NUM_INS];
    this->colors = new glm::vec3[NUM_INS];
    std::cout << "Volume: " << pow(MASS * NUM_INS / RESTING_DENSITY, 1.0 / 3.0) << std::endl;
    genUniformVec3Array(positions, NUM_INS_DIM, 5.0f);

    for (int i = 0; i < NUM_INS; i++) {
        colors[i] = glm::vec3(1.0f);
    }

    // CUDA memory allocation for particle data
    cudaMalloc((void**)&d_positions, NUM_INS * sizeof(glm::vec3));
    cudaMalloc((void**)&d_velocities, NUM_INS * sizeof(glm::vec3));
    cudaMalloc((void**)&d_forces, NUM_INS * sizeof(glm::vec3));
    cudaMalloc((void**)&d_densities, NUM_INS * sizeof(float));
    cudaMalloc((void**)&d_pressures, NUM_INS * sizeof(float));

    cudaMemcpy(d_positions, positions, NUM_INS * sizeof(glm::vec3), cudaMemcpyHostToDevice);
    cudaMemcpy(d_velocities, velocities, NUM_INS * sizeof(glm::vec3), cudaMemcpyHostToDevice);
    cudaMemcpy(d_forces, forces, NUM_INS * sizeof(glm::vec3), cudaMemcpyHostToDevice);

    // Rendering
    setupVAO();
}

void Particles::update()
{
    // Launch kernels to compute densities, pressures, forces, update particles and resolve collisions
    int blockSize = 256; // 256 threads per block
    int numBlocks = (NUM_INS + blockSize - 1) / blockSize;

    // Calculate density and pressure in parallel
    calculateDensityAndPressureKernel<<<numBlocks, blockSize>>>(d_positions, d_densities, d_pressures, NUM_INS);

    // Apply forces (pressure, viscosity, gravity) in parallel
    applyForcesKernel<<<numBlocks, blockSize>>>(d_positions, d_velocities, d_forces, d_pressures, d_densities, NUM_INS);

    // Update particles (positions, velocities)
    updateParticlesKernel<<<numBlocks, blockSize>>>(d_positions, d_velocities, d_forces, d_densities, NUM_INS);

    // Resolve collisions between particles
    resolveCollisionsKernel<<<numBlocks, blockSize>>>(d_positions, d_velocities, NUM_INS);

    // Copy the updated particle positions back to host
    cudaMemcpy(positions, d_positions, NUM_INS * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(velocities, d_velocities, NUM_INS * sizeof(glm::vec3), cudaMemcpyDeviceToHost);
    cudaMemcpy(forces, d_forces, NUM_INS * sizeof(glm::vec3), cudaMemcpyDeviceToHost);

    // Update VBO for rendering
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3), NUM_INS * sizeof(glm::vec3), colors);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Particles::Draw()
{
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", Radius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, NUM_INS);
    glBindVertexArray(0);
}

void Particles::setPositions(glm::vec3 *positions)
{
    this->positions = positions;
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), positions);
    glBindVertexArray(0);

    for (int i = 0; i < NUM_INS; i++)
    {
        velocities[i] = glm::vec3(0.0f);
        colors[i] = glm::vec3(1.0f);
    }
}

void Particles::reset()
{
    genUniformVec3Array(positions, NUM_INS_DIM, 5.0f);
    for (int i = 0; i < NUM_INS; i++)
    {
        colors[i] = glm::vec3(1.0f);
        velocities[i] = glm::vec3(0.0f);
        forces[i] = glm::vec3(0.0f);
    }
}

Particles::~Particles()
{
    // Clean up CUDA memory
    cudaFree(d_positions);
    cudaFree(d_velocities);
    cudaFree(d_forces);
    cudaFree(d_densities);
    cudaFree(d_pressures);

    delete[] positions;
    delete[] colors;
}
