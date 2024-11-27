#include <glad/glad.h>
#include <particles.hpp>
#include <config.hpp>
#include <cuda_runtime.h>
#include <bits/stdc++.h>

// External CUDA functions
extern void launchUpdateParticles(float* d_pos, float* d_vel, float* d_acc, int numParticles, float deltaTime);

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

Particles::Particles(Shader* shader) : shader(shader) {
    // Initialize CPU data
    h_positions.resize(MAX_PARTICLES * 3);
    h_velocities.resize(MAX_PARTICLES * 3);
    h_accelerations.resize(MAX_PARTICLES * 3);
    
    // Initialize particles
    reset();
    
    // Initialize CUDA
    initCUDA();
    
    // Setup OpenGL buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * h_positions.size(), h_positions.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void Particles::setupVAO()
{
    // VAO : Vertex Array Object
    glGenVertexArrays(1, &VAO);
    // VBO : Vertex Buffer Object
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3) * 2, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), d_positions);
    glBufferSubData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3), NUM_INS * sizeof(glm::vec3), d_colors);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)(NUM_INS * sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Particles::update() {
    // Launch CUDA kernel
    launchUpdateParticles(d_positions, d_velocities, d_accelerations, MAX_PARTICLES, 0.016f);
    
    // Copy back results for rendering
    cudaMemcpy(h_positions.data(), d_positions, sizeof(float) * h_positions.size(), cudaMemcpyDeviceToHost);
    
    // Update OpenGL buffer
    updateGPUBuffers();
}

void Particles::updateGPUBuffers() {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * h_positions.size(), h_positions.data());
}

void Particles::Draw() {
    shader->use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, MAX_PARTICLES);
}

void Particles::reset() {
    // Initialize particle positions and velocities
    for (int i = 0; i < MAX_PARTICLES; i++) {
        // Random initial positions
        h_positions[i*3] = static_cast<float>(rand()) / RAND_MAX * 10.0f;
        h_positions[i*3+1] = static_cast<float>(rand()) / RAND_MAX * 10.0f;
        h_positions[i*3+2] = static_cast<float>(rand()) / RAND_MAX * 10.0f;
        
        // Zero initial velocities and accelerations
        h_velocities[i*3] = h_velocities[i*3+1] = h_velocities[i*3+2] = 0.0f;
        h_accelerations[i*3] = h_accelerations[i*3+1] = h_accelerations[i*3+2] = 0.0f;
    }
    
    // Update GPU data
    if (d_positions != nullptr) {
        cudaMemcpy(d_positions, h_positions.data(), sizeof(float) * h_positions.size(), cudaMemcpyHostToDevice);
        cudaMemcpy(d_velocities, h_velocities.data(), sizeof(float) * h_velocities.size(), cudaMemcpyHostToDevice);
        cudaMemcpy(d_accelerations, h_accelerations.data(), sizeof(float) * h_accelerations.size(), cudaMemcpyHostToDevice);
    }
}

Particles::~Particles() {
    cleanupCUDA();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Particles::initCUDA() {
    cudaMalloc(&d_positions, sizeof(float) * h_positions.size());
    cudaMalloc(&d_velocities, sizeof(float) * h_velocities.size());
    cudaMalloc(&d_accelerations, sizeof(float) * h_accelerations.size());
    
    cudaMemcpy(d_positions, h_positions.data(), sizeof(float) * h_positions.size(), cudaMemcpyHostToDevice);
    cudaMemcpy(d_velocities, h_velocities.data(), sizeof(float) * h_velocities.size(), cudaMemcpyHostToDevice);
    cudaMemcpy(d_accelerations, h_accelerations.data(), sizeof(float) * h_accelerations.size(), cudaMemcpyHostToDevice);
}

void Particles::cleanupCUDA() {
    cudaFree(d_positions);
    cudaFree(d_velocities);
    cudaFree(d_accelerations);
}
