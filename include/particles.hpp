#ifndef PARTICLES_HPP
#define PARTICLES_HPP

#include <glm/glm.hpp>
#include <shader.hpp> // Assuming you have a Shader class
#include <cuda_runtime.h>

// Particle class definition
class Particles {
public:
    // Particle data
    glm::vec3 *positions;
    glm::vec3 *velocities;
    glm::vec3 *forces;
    glm::vec3 *colors;
    float *densities;
    float *pressures;

    // Shader for rendering
    Shader *shader;
    
    // OpenGL buffers and VAO
    GLuint VAO, VBO;

    // Constructor and Destructor
    Particles(Shader *const shader);
    ~Particles();

    // Function declarations
    void update();
    void Draw();
    void reset();
    void setPositions(glm::vec3 *positions);
    
    // CUDA-specific setup
    void setupCUDA();

    // Utility functions
    void genUniformVec3Array(glm::vec3 *arr, int n, float scale = 1.0f);
    inline glm::vec3 getRandVec3();
    void setupVAO();
    
private:
    // CUDA memory pointers
    glm::vec3 *d_positions, *d_velocities, *d_forces;
    float *d_densities, *d_pressures;
};

#endif // PARTICLES_HPP
