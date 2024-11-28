#pragma once

#include <glm/glm.hpp>
#include <shader.hpp> // Assuming you have a Shader class

#ifdef __CUDACC__
// Struct for Particle Data
struct __align__(16) Particle
#else
struct Particle
#endif
{
    glm::vec3 position; // Position of the particle (12 bytes)
    float padding1;     // Padding to align to 16 bytes
    glm::vec3 velocity; // Velocity of the particle (12 bytes)
    float padding2;     // Padding to align to 16 bytes
    glm::vec3 force;    // Force acting on the particle (12 bytes)
    float density;      // Density of the particle (4 bytes)
    glm::vec3 color;    // Color of the particle (12 bytes)
    float pressure;     // Pressure of the particle (4 bytes)

    // Constructor
    #ifdef __CUDACC__
    __host__ __device__ Particle()
        : position(0.0f), velocity(0.0f), force(0.0f), density(0.0f), color(1.0f), pressure(0.0f)  {}
    #else
    Particle()
        : position(0.0f), velocity(0.0f), force(0.0f), density(0.0f), color(1.0f), pressure(0.0f)  {}
    #endif
};

// Particle class definition
class ParticleSystem
{
public:
    // Particle data
    Particle *h_particles;
    
    // Shader for rendering
    Shader *shader;

    // Constructor and Destructor
    ParticleSystem(Shader *const shader);
    ~ParticleSystem();

    // Function declarations
    void updateGPU();
    void Draw();
    void reset();

private:
    GLuint VAO, VBO; // OpenGL buffers and VAO
    Particle *d_particles; // CUDA memory pointers
    void setupVAO(); 
};