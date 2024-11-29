#include <glad/glad.h>
#include <config.hpp>
#include <bits/stdc++.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <cmath>
#include <time.h>

#include <shader.hpp>
#include <vec3.hpp>
#include <particle_system.hpp>
#include <smoothing_kernels.hpp>

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
        Vec3 r = particles[j].position - particles[i].position;
        float r2 = r.dot(r);
        particles[i].density += MASS * poly6Kernel(r2);
    }
    // particles[i].pressure = BULK_MODULUS * fmaxf((particles[i].density - RESTING_DENSITY), 0.0f); // Desburn and Cani
    particles[i].nearPressure = BULK_MODULUS_NEAR * particles[i].density;
    particles[i].pressure = (BULK_MODULUS * RESTING_DENSITY / 7.0f) * (powf(particles[i].density / RESTING_DENSITY, 7.0f) - 1.0f); // Monaghan
}

__global__ void calculateForcesKernel(Particle *particles, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;
    particles[i].force = Vec3(0.0f, -gravity * MASS, 0.0f); //+ (particles[i].position - Vec3(5, 2.5, 2.5)).cross(Vec3(0.0f, 0.05f, 0.0f)); // Gravity
    for (int j = 0; j < numParticles; j++)
    {
        if (i == j)
            continue;
        Vec3 r = particles[i].position - particles[j].position;
        float sqrt_r = r.length();
        particles[i].force += -(MASS / (2.0f * particles[j].density + DIVISON_EPSILON)) * ((particles[i].pressure + particles[j].pressure) * spikyGradient(r, sqrt_r) + (particles[i].nearPressure + particles[j].nearPressure) * spikyGradientNear(r, sqrt_r)); // Pressure term
        particles[i].force += mu * MASS * (particles[j].velocity - particles[i].velocity) / (particles[j].density + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r);                                                                                               // Viscosity term
    }
}

__global__ void updateParticlesKernel(Particle *particles, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    particles[i].velocity += particles[i].force / (particles[i].density + DIVISON_EPSILON) * dt;

    if (particles[i].velocity.length() > MAX_VELOCITY)
    {
        particles[i].velocity = particles[i].velocity.normalize() * MAX_VELOCITY;
    }

    particles[i].position += particles[i].velocity * dt +
                             0.5f * dt * dt / (particles[i].density + DIVISON_EPSILON);

    // Boundary conditions (similar to original code)
    particles[i].position.x = fminf(fmaxf(particles[i].position.x, 0.0f + EPSILON), 10.0f - EPSILON);
    particles[i].velocity.x *= (particles[i].position.x == 0.0f + EPSILON || particles[i].position.x == 10.0f - EPSILON) ? -COEFF_RESTITUTION : 1.0f;

    particles[i].position.y = fminf(fmaxf(particles[i].position.y, 0.0f + EPSILON), 10.0f - EPSILON);
    particles[i].velocity.y *= (particles[i].position.y == 0.0f + EPSILON || particles[i].position.y == 10.0f - EPSILON) ? -COEFF_RESTITUTION : 1.0f;

    particles[i].position.z = fminf(fmaxf(particles[i].position.z, 0.0f + EPSILON), 5.0f - EPSILON);
    particles[i].velocity.z *= (particles[i].position.z == 0.0f + EPSILON || particles[i].position.z == 5.0f - EPSILON) ? -COEFF_RESTITUTION : 1.0f;

    // Color update based on density
    float scale = fminf(fmaxf(particles[i].pressure / 2.0f, 0.0f), 1.0f);
    // float scale = fminf(fmaxf(particles[i].density / RESTING_DENSITY, 0.0f), 1.0f);
    particles[i].color = Vec3(0.0f, 1.0f, 0.0f) * (1.0f - scale) + Vec3(1.0f, 0.0f, 0.0f) * scale;
    // Vec3 vorticity = Vec3(0.0f);
    // for (int j = 0; j < numParticles; j++)
    // {
    //     if (i == j)
    //         continue;
    //     Vec3 r = particles[j].position - particles[i].position;
    //     float sqrt_r = r.length();
    //     vorticity += r.cross(particles[j].velocity - particles[i].velocity) * poly6Kernel(sqrt_r);
    // }
    // float length = 2.0f * vorticity.length();
    // particles[i].color = Vec3(0.0f, 0.0f, 1.0f) * (1 - 2 * length) + Vec3(1.0f, 0.0f, 0.0f) * (2 * length - 1) + Vec3(0.0f, 1.0f, 0.0f) * 4.0f * length * (1 - length);
}

__global__ void resolveCollisionsKernel(Particle *particles, int numParticles)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    for (int j = 0; j < numParticles; j++)
    {
        if (i == j)
            continue;
        float dist = (particles[i].position - particles[j].position).length();
        if (dist < 2.0f * Radius)
        {
            Vec3 normal = (particles[i].position - particles[j].position).normalize();
            particles[i].position += normal * (Radius - 0.5f * dist);
            particles[j].position -= normal * (Radius - 0.5f * dist);

            // Velocity restitution (elastic collision)
            particles[i].velocity -= particles[i].velocity.dot(normal) * normal * (1 + COEFF_RESTITUTION);
            particles[j].velocity -= particles[j].velocity.dot(normal) * normal * (1 + COEFF_RESTITUTION);
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
    resetParticles();

    // Rendering
    setupVAO();
}

__host__ void ParticleSystem::setupVAO()
{
    glGenVertexArrays(1, &VAO); // VAO : Vertex Array Object
    glGenBuffers(1, &VBO);      // VBO : Vertex Buffer Object

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_INS * sizeof(Particle), h_particles, GL_DYNAMIC_DRAW);

    // Register VBO with CUDA
    cudaGraphicsGLRegisterBuffer(&cudaVBOResource, VBO, cudaGraphicsMapFlagsNone);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // Map OpenGL buffer for CUDA
    size_t size;
    cudaGraphicsMapResources(1, &cudaVBOResource);
    cudaGraphicsResourceGetMappedPointer((void **)&d_particles, &size, cudaVBOResource);
}

__host__ void ParticleSystem::updateParticles()
{
    // Launch kernels to compute densities, pressures, forces, update particles and resolve collisions
    int blockSize = 256; // 256 threads per block
    int numBlocks = (NUM_INS + blockSize - 1) / blockSize;

    // Launch kernels
    calculateDensityAndPressureKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    // cudaDeviceSynchronize();
    calculateForcesKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    // cudaDeviceSynchronize();
    updateParticlesKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    // cudaDeviceSynchronize();
    resolveCollisionsKernel<<<numBlocks, blockSize>>>(d_particles, NUM_INS);
    // cudaDeviceSynchronize();
    
    // Unmap the buffer
    // cudaGraphicsUnmapResources(1, &cudaVBOResource);

    // // Copy the updated particle positions back to host
    // cudaMemcpy(h_particles, d_particles, NUM_INS * sizeof(Particle), cudaMemcpyDeviceToHost);

    // // Update VBO for rendering
    // glBindVertexArray(VAO);
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(Particle), h_particles);
    // glEnableVertexAttribArray(0);
    // glBindVertexArray(0);
}

__host__ void ParticleSystem::renderParticles()
{
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", Radius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, NUM_INS);
    glBindVertexArray(0);
}

__host__ void ParticleSystem::resetParticles()
{
    for (int i = 0; i < NUM_INS_DIM; i++)
    {
        for (int j = 0; j < NUM_INS_DIM; j++)
        {
            for (int k = 0; k < NUM_INS_DIM; k++)
            {
                h_particles[i * NUM_INS_DIM * NUM_INS_DIM + j * NUM_INS_DIM + k] = Particle();
                float factor = 5.0f / (NUM_INS_DIM - 1);
                h_particles[i * NUM_INS_DIM * NUM_INS_DIM + j * NUM_INS_DIM + k].position = Vec3(i * factor, j * factor, k * factor);
            }
        }
    }

    // Copy data to host memory
    cudaMemcpy(d_particles, h_particles, NUM_INS * sizeof(Particle), cudaMemcpyHostToDevice);
}

ParticleSystem::~ParticleSystem()
{
    // Clean up device memory
    // cudaFree(d_particles);
    cudaGraphicsUnregisterResource(cudaVBOResource);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    // Clean up host memory
    delete[] h_particles;
}
