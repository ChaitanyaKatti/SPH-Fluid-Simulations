#include <glad/glad.h>
#include <config.hpp>
#include <bits/stdc++.h>
#include <cuda_runtime.h>
#include <particles.cuh>
#include <vector_operators.cuh>

#include <cuda_runtime.h>
#include <glm/glm.hpp>
#include <cmath>
#include <config.hpp>
#include <time.h>

// Kernel functions for SPH simulation
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

__global__ void calculateDensityAndPressureKernel(Particle *particles, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    particles[i].density = 0.0f;
    for (int j = 0; j < numParticles; j++)
    {
        if (i == j)
            continue;
        glm::vec3 r = particles[j].position - particles[i].position;
        float r2 = glm::dot(r, r);
        particles[i].density += MASS * poly6Kernel(r2);
    }
    particles[i].pressure = BULK_MODULUS * fmaxf((particles[i].density - RESTING_DENSITY), 0.0f);
}

__global__ void calculateForcesKernel(Particle *particles, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    // particles[i].force = glm::vec3(0.0f, -MASS * gravity, 0.0f); // Gravity

    for (int j = 0; j < numParticles; j++)
    {
        if (i == j)
            continue;
        glm::vec3 r = particles[i].position - particles[j].position;
        float sqrt_r = glm::length(r);
        // particles[i].force += -MASS * (particles[i].pressure + particles[j].pressure) * spikyGradient(r, sqrt_r);                                                  // Pressure term
        // particles[i].force += mu * MASS * (particles[j].velocity - particles[i].velocity) / (particles[j].density + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
    }
}

__global__ void updateParticlesKernel(Particle *particles, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    // Update velocities and positions
    particles[i].velocity += dt / (particles[i].density + DIVISON_EPSILON);
    if (glm::length(particles[i].velocity) > MAX_VELOCITY)
    {
        particles[i].velocity = glm::normalize(particles[i].velocity) * MAX_VELOCITY;
    }
    particles[i].position += dt * particles[i].velocity + 0.5f * dt * dt / (particles[i].density + DIVISON_EPSILON);

    // Boundary conditions
    if (particles[i].position.x < 0.0f)
    {
        particles[i].position.x = 0.0f + EPSILON;
        particles[i].velocity.x *= -COEFF_RESTITUTION;
    }
    if (particles[i].position.x > 10.0f)
    {
        particles[i].position.x = 10.0f - EPSILON;
        particles[i].velocity.x *= -COEFF_RESTITUTION;
    }
    if (particles[i].position.y < 0.0f)
    {
        particles[i].position.y = 0.0f + EPSILON;
        particles[i].velocity.y *= -COEFF_RESTITUTION;
    }
    if (particles[i].position.y > 10.0f)
    {
        particles[i].position.y = 10.0f - EPSILON;
        particles[i].velocity.y *= -COEFF_RESTITUTION;
    }
    if (particles[i].position.z < 0.0f)
    {
        particles[i].position.z = 0.0f + EPSILON;
        particles[i].velocity.z *= -COEFF_RESTITUTION;
    }
    if (particles[i].position.z > 5.0f)
    {
        particles[i].position.z = 5.0f - EPSILON;
        particles[i].velocity.z *= -COEFF_RESTITUTION;
    }

    // Update colors based on particle density
    float density = particles[i].density;
    particles[i].color = glm::vec3(1.0f, 0.0f, 0.0f) * (1.0f - fminf(fmaxf(density / RESTING_DENSITY, 0.0f), 1.0f)) +
                         glm::vec3(0.0f, 1.0f, 0.0f) * fminf(fmaxf(density / RESTING_DENSITY, 0.0f), 1.0f);
}

__global__ void resolveCollisionsKernel(Particle *particles, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    for (int j = i + 1; j < numParticles; j++)
    {
        float dist = glm::length(particles[i].position - particles[j].position);
        if (dist < 2.0f * Radius)
        {
            glm::vec3 normal = glm::normalize(particles[i].position - particles[j].position);
            particles[i].position += normal * (Radius - 0.5f * dist);
            particles[j].position -= normal * (Radius - 0.5f * dist);

            // Velocity restitution (elastic collision)
            particles[i].velocity -= glm::dot(particles[i].velocity, normal) * normal * (1 + COEFF_RESTITUTION);
            particles[j].velocity -= glm::dot(particles[j].velocity, normal) * normal * (1 + COEFF_RESTITUTION);
        }
    }
}

__host__ ParticleSystem::ParticleSystem(Shader *const shader) : shader(shader)
{
    // Initialize arrays for SPH
    this->h_particles = new Particle[NUM_INS];
    std::cout << "Volume: " << pow(MASS * NUM_INS / RESTING_DENSITY, 1.0 / 3.0) << std::endl;

    // CUDA memory allocation for particle data
    cudaMalloc((void **)&d_particles, NUM_INS * sizeof(Particle));

    // Initialize particle data
    reset();

    // Rendering
    setupVAO();
}

__host__ void ParticleSystem::setupVAO()
{
    // VAO : Vertex Array Object
    glGenVertexArrays(1, &VAO);
    // VBO : Vertex Buffer Object
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_INS * sizeof(Particle), h_particles, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

__host__ void ParticleSystem::updateGPU()
{
    std::cout << "Position: " << h_particles[0].position.x << " " << h_particles[0].position.y << " " << h_particles[0].position.z << std::endl;
    std::cout << "velocity: " << h_particles[0].velocity.x << " " << h_particles[0].velocity.y << " " << h_particles[0].velocity.z << std::endl;
    std::cout << "Force: " << h_particles[0].force.x << " " << h_particles[0].force.y << " " << h_particles[0].force.z << std::endl;
    std::cout << "Density: " << h_particles[0].density << std::endl;
    std::cout << "Pressure: " << h_particles[0].pressure << std::endl;
    std::cout << "----------" << std::endl;

    // Launch kernels to compute densities, pressures, forces, update particles and resolve collisions
    int blockSize = 256; // 256 threads per block
    int numBlocks = (NUM_INS + blockSize - 1) / blockSize;
    
    // Launch kernels
    // cudaDeviceSynchronize();
    // calculateDensityAndPressureKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    // cudaDeviceSynchronize();
    // calculateForcesKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    // cudaDeviceSynchronize();
    // updateParticlesKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    // cudaDeviceSynchronize();
    resolveCollisionsKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    cudaDeviceSynchronize();

    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess)
    {
        std::cerr << "CUDA error in updateGPU: " << cudaGetErrorString(error) << std::endl;
    }

    // Copy the updated particle positions back to host
    cudaMemcpy(h_particles, d_particles, NUM_INS * sizeof(Particle), cudaMemcpyDeviceToHost);

    // Update VBO for rendering
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(Particle), h_particles);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

__host__ void ParticleSystem::Draw()
{
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", Radius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, NUM_INS);
    glBindVertexArray(0);
}

__host__ void ParticleSystem::reset()
{
    for (int i = 0; i < NUM_INS_DIM; i++)
    {
        for (int j = 0; j < NUM_INS_DIM; j++)
        {
            for (int k = 0; k < NUM_INS_DIM; k++)
            {
                h_particles[i * NUM_INS_DIM * NUM_INS_DIM + j * NUM_INS_DIM + k] = Particle();
                float factor = 5.0f / (NUM_INS_DIM - 1);
                h_particles[i * NUM_INS_DIM * NUM_INS_DIM + j * NUM_INS_DIM + k].position = glm::vec3(i * factor, j * factor, k * factor);
            }
        }
    }

    // Copy data to host memory
    cudaMemcpy(d_particles, h_particles, NUM_INS * sizeof(Particle), cudaMemcpyHostToDevice);
}

ParticleSystem::~ParticleSystem()
{
    // Clean up CUDA memory
    cudaFree(d_particles);
    // Clean up host memory
    delete[] h_particles;
}
