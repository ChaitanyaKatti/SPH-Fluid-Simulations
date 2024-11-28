#pragma once

#include <glm/glm.hpp>
#include <shader.hpp> // Assuming you have a Shader class

// Particle class definition
class ParticleSystem
{
public:
    // Particle data
    glm::vec3 *h_positions, *h_colors;
    
    // Shader for rendering
    Shader *shader;

    // Constructor and Destructor
    ParticleSystem(Shader *const shader);
    ~ParticleSystem();

    // Function declarations
    void updateGPU();
    void Draw();
    void reset();
    void setPositions(glm::vec3 *h_positions);

private:
    // OpenGL buffers and VAO
    GLuint VAO, VBO;
    // CUDA memory pointers
    glm::vec3 *d_positions, *d_colors, *d_velocities, *d_forces;
    float *d_densities, *d_pressures;
    // Utility functions
    void genUniformVec3Array(glm::vec3 *arr, int n, float scale = 1.0f);
    inline glm::vec3 getRandVec3();
    void setupVAO();
};