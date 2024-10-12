#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <shader.hpp>
#include <texture.hpp>
#include <config.hpp>
#include <spatialgrid.hpp>

class Particles
{
private:
    float mass;
    float resting_density;
    float radius;
    glm::vec3 *positions;
    // glm::vec3 positions[NUM_INS];
    glm::vec3 *colors;
    // glm::vec3 colors[NUM_INS];
    int num_points;
    SpatialGrid* grid;
    Shader* shader;
    
    unsigned int VAO, VBO;
    
    // Smoothed Particle Hydrodynamics
    glm::vec3 *velocities;
    // glm::vec3 velocities[NUM_INS];
    float *densities;
    // float densities[NUM_INS];
    float *pressures;
    // float pressures[NUM_INS];
    glm::vec3 *forces;
    // glm::vec3 forces[NUM_INS];


    void setupParticles();
    void calculateDensityAndPressure();
    void applyForces(float dt);
    void hashCoords(int i, int j, int k);
public:
    Particles(const float mass, const float resting_density, const float radius, int num_points, SpatialGrid* grid, Shader* const shader); 
    void update(float dt);
    void Draw();
    void setPositions(glm::vec3* positions);
    void resetParticles();
    ~Particles();
};