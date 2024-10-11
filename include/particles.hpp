#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <shader.hpp>
#include <texture.hpp>

class Particles
{
private:
    float mass;
    float resting_density;
    float radius;
    glm::vec3 *positions;
    glm::vec3 *colors;
    int num_points;
    Shader* shader;
    
    unsigned int VAO, VBO;
    
    // Smoothed Particle Hydrodynamics
    glm::vec3 *velocities;
    float *densities;
    float *pressures;
    glm::vec3 *forces;

    void setupParticles();
    void calculateDensityAndPressure();
    void applyForces(float dt);

public:
    Particles(const float mass, const float resting_density, const float radius, glm::vec3* positions, glm::vec3* colors, int num_points, Shader* const shader); 
    void update(float dt);
    void Draw();
    void setPositions(glm::vec3* positions);
};