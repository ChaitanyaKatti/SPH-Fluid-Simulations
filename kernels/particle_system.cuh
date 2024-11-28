#pragma once

#include <math_vector.cuh>
#include <shader.hpp>

#ifdef __CUDACC__
struct __align__(16) Particle
#else
struct Particle
#endif
{
    Vec3 position;  // Position of the particle
    float padding1; // Padding to align to 16 bytes
    Vec3 velocity;  // Velocity of the particle
    float padding2; // Padding to align to 16 bytes
    Vec3 force;     // Force acting on the particle
    float density;  // Density of the particle
    Vec3 color;     // Color of the particle
    float pressure; // Pressure of the particle

// Constructor
#ifdef __CUDACC__
    __host__ __device__ Particle()
#else
    Particle()
#endif
        : position(0.0f), velocity(0.0f), force(0.0f),
          density(0.0f), color(1.0f), pressure(0.0f)
    {
    }
};

class ParticleSystem
{
public:
    Particle *h_particles;
    Shader *shader;

    ParticleSystem(Shader *const shader);
    ~ParticleSystem();

    void updateParticles();
    void renderParticles();
    void resetParticles();

private:
    GLuint VAO, VBO;
    Particle *d_particles;
    void setupVAO();
};