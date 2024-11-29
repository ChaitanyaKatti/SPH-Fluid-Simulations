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

    int* startIndex;
    int* stopIndex;
    int* indexArray;

    // Smoothed Particle Hydrodynamics
    float *densities;
    float *pressures;
    float *nearPressures;
    glm::vec3 *forces;
    glm::vec3 *velocities;

    void setupVAO();
    int hash(glm::vec3 p);
    void updateHash();
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