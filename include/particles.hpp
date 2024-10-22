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
    int hash_table_size;
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

    // hash table
    int *start_index;  // start index of each cell in particle map
    int *end_index;    // end index of each cell in particle map
    int *particleMap; // contains index to particle array

    void setupVAO();
    void calculateDensityAndPressure();
    void applyForces(float dt);
    int hashCoords(glm::ivec3 cellId);
    void updateHashTable();
public:
    Particles(const float mass, const float resting_density, const float radius, int num_points, int hash_table_size, SpatialGrid* grid, Shader* const shader); 
    void update(float dt);
    void Draw();
    void setPositions(glm::vec3* positions);
    void resetParticles();
    ~Particles();
};