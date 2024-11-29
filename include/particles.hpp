#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <shader.hpp>

class Particles
{
private:
    glm::vec3 *positions;
    glm::vec3 *colors;
    Shader *shader;

    unsigned int VAO, VBO;

    // Smoothed Particle Hydrodynamics
    float *densities;
    float *pressures;
    float *nearPressures;
    glm::vec3 *forces;
    glm::vec3 *velocities;

    void setupVAO();
    void calculateDensityAndPressure();
    void applyForces();
    void resolveCollisions();

public:
    Particles(Shader *const shader);
    void update();
    void Draw();
    void setPositions(glm::vec3 *positions);
    void reset();
    ~Particles();
};