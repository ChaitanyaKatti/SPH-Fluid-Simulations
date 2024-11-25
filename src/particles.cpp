#include <glad/glad.h>
#include <iostream>
#include <algorithm>

#include <particles.hpp>
#include <utils.hpp>
#include <config.hpp>

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;

inline float poly6Kernel(float r2)
{
    if (r2 < h2)
    {
        return 315.0f / (64.0f * M_PI * glm::pow(h, 9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r)
{
    if (sqrt_r < h)
    {
        // if (sqrt_r < 0.0001f){
        // return (float)(-45.0f / (M_PI * h6) * h2 ) * glm::normalize(getRandVec3());
        // }
        return (float)(-45.0f / (M_PI * h6) * glm::pow(h - sqrt_r, 2)) * r / (sqrt_r + DIVISON_EPSILON);
    }
    return glm::vec3(0.0f);
}

inline float viscosityLaplacian(float sqrt_r)
{
    if (sqrt_r < h)
    {
        return 45.0f / (M_PI * h6) * (h - sqrt_r);
    }
    return 0.0f;
}

Particles::Particles(const float mass, const float resting_density, const float radius, int num_points, int hash_table_size, SpatialGrid* grid, Shader *const shader) : mass(mass), resting_density(resting_density), radius(radius), num_points(num_points), hash_table_size(hash_table_size), grid(grid), shader(shader)
{
    std::cout << "Hash table size: " << hash_table_size << std::endl;
    positions = new glm::vec3[num_points];
    colors = new glm::vec3[num_points];

    velocities = new glm::vec3[num_points];
    densities = new float[num_points];
    pressures = new float[num_points];
    forces = new glm::vec3[num_points];

    start_index = new int[hash_table_size];
    end_index = new int[hash_table_size];
    particleMap = new int[num_points];

    for (int i = 0; i < num_points; i++)
    {
        velocities[i] = glm::vec3(0.0f);
    }

    genUniformVec3Array(positions, NUM_INS_DIM, 5.0f);
    genUniformVec3Array(colors, NUM_INS_DIM, 1.0f);
    setupVAO();
}

void Particles::setupVAO()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, num_points * sizeof(glm::vec3) * 2, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, num_points * sizeof(glm::vec3), num_points * sizeof(glm::vec3), colors);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)(num_points * sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Particles::update(float dt)
{
    updateHashTable();
    calculateDensityAndPressure();
    applyForces(dt);

    // Update VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, num_points * sizeof(glm::vec3), num_points * sizeof(glm::vec3), colors);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Particles::Draw()
{
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", radius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, num_points);
    glBindVertexArray(0);
}

void Particles::calculateDensityAndPressure()
{
// #pragma omp parallel for
    for (int i = 0; i < num_points; i++)
    {
        densities[i] = 0.0f;
        
        // Calculate density, O(n^2) complexity
        // for (int j = 0; j < num_points; j++)
        // {
        //     if (i != j)
        //     {
        //         glm::vec3 r = positions[i] - positions[j];
        //         float r2 = glm::dot(r, r);
        //         densities[i] += mass * poly6Kernel(r2);
        //     }
        // }

        // Calculate density, O(n) complexity
        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                for (int z = -1; z <= 1; z++)
                {
                    // glm::ivec3 cellId = grid->getCellIdWithOffset(positions[i], glm::ivec3(x, y, z));
                    glm::ivec3 cellId = grid->getCellId(positions[i]) + glm::ivec3(x, y, z);
                    int hash_neigh = hashCoords(cellId);
                    for (int j = start_index[hash_neigh]; j < end_index[hash_neigh]; j++)
                    {
                        int neighIndex = particleMap[j];
                        if (i != neighIndex)
                        {
                            glm::vec3 r = positions[i] - positions[neighIndex];
                            float r2 = glm::dot(r, r);
                            if (r2 < h2)
                            {   
                                densities[i] += mass * poly6Kernel(r2);
                                colors[i] = glm::vec3(1.0f, 1.0f, 1.0f);
                                colors[neighIndex] = glm::vec3(1.0f, 1.0f, 1.0f);
                            }
                            else{
                                // Resolve hash collision
                                colors[i] = glm::vec3(1.0f, 1.0f, 0.0f);
                                colors[neighIndex] = glm::vec3(1.0f, 1.0f, 0.0f);
                            }
                        }
                    }
                }
            }
        }

        pressures[i] = k * (densities[i] - resting_density);
    }
}

void Particles::applyForces(float dt)
{
// #pragma omp parallel for
    for (int i = 0; i < num_points; i++)
    {
        forces[i] = glm::vec3(0.0f, -mass * g, 0.0f); // Gravity

        // Calculate forces, O(n^2) complexity
        // for (int j = 0; j < num_points; j++)
        // {
        //     if (i != j)
        //     {
        //         glm::vec3 r = positions[i] - positions[j];
        //         float sqrt_r = glm::length(r);
        //         forces[i] += -mass * (pressures[i] + pressures[j]) / (2.0f * densities[j] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);  // Pressure term
        //         forces[i] += mu * mass * (velocities[j] - velocities[i]) / (densities[j] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
        //     }
        // }

        // Calculate forces, O(n) complexity
        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                for (int z = -1; z <= 1; z++)
                {
                    glm::ivec3 cellId = grid->getCellId(positions[i]) + glm::ivec3(x, y, z);
                    int hash_neigh = hashCoords(cellId);
                    for (int j = start_index[hash_neigh]; j < end_index[hash_neigh]; j++)
                    {
                        int neigh = particleMap[j];
                        if (i != neigh)
                        {
                            glm::vec3 r = positions[i] - positions[neigh];
                            float sqrt_r = glm::length(r);
                            // if (sqrt_r < h)
                            // {
                                forces[i] += -mass * (pressures[i] + pressures[neigh]) / (2.0f * densities[neigh] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);  // Pressure term
                                forces[i] += mu * mass * (velocities[neigh] - velocities[i]) / (densities[neigh] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
                            // }
                            // else{
                                // Resolve hash collision
                                // std::cout << "Hash collision detected" << std::endl;
                            // }
                        }
                    }
                }
            }
        }

        // Update velocities and positions
        velocities[i] += dt * forces[i] / (densities[i] + DIVISON_EPSILON);
        if (glm::length(velocities[i]) > MAX_VELOCITY)
        {
            velocities[i] = glm::normalize(velocities[i]) * MAX_VELOCITY;
        }
        positions[i] += dt * velocities[i];// + 0.5f * dt * dt * forces[i] / (densities[i] + DIVISON_EPSILON);

        
        // Apply boundary conditions
        if (positions[i][1] < 0.0f)
        {
            positions[i][1] = 0.0f;
            velocities[i][1] = -COEFF_RESTITUTION * velocities[i][1];
        }
        grid->transformPositionVelocity(positions[i], velocities[i]);

        // Update colors
        // float speed = glm::length(velocities[i]) / 5.0f;
        // colors[i] = colorParticleBGR1(speed);
    }
}

void Particles::setPositions(glm::vec3 *positions)
{
    positions = positions;
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(glm::vec3), positions);
    glBindVertexArray(0);

    for (int i = 0; i < num_points; i++)
    {
        velocities[i] = glm::vec3(0.0f);
        colors[i] = glm::vec3(1.0f);
    }
}

void Particles::resetParticles()
{
    genUniformVec3Array(positions, NUM_INS_DIM, 5.0f);
    genUniformVec3Array(colors, NUM_INS_DIM, 1.0f);
    setPositions(positions);
}

int Particles::hashCoords(glm::ivec3 cellId)
{
    int hash = (cellId.x * 92837111) ^ (cellId.y * 689287499) ^ (cellId.z * 283923481); // Credit Matthias Muller
    return abs(hash % hash_table_size);
    // glm::ivec3 num_cells = grid->getNumCells();
    // int hash = (cellId.x + num_cells.x * (cellId.y + num_cells.y * cellId.z)) % hash_table_size;
    // return hash;
}

void Particles::updateHashTable()
{
    std::fill(start_index, start_index + hash_table_size, 0);
    std::fill(end_index, end_index + hash_table_size, 0);
    std::fill(particleMap, particleMap + num_points, 0);
    // For every particle, add +1 at the hash index
    for (int i = 0; i < num_points; i++)
    {
        glm::ivec3 cellId = grid->getCellId(positions[i]);
        int hashedCellId = hashCoords(cellId);
        end_index[hashedCellId]++;
    }

    // Prefix sum
    for (int i = 1; i < hash_table_size; i++)
    {
        end_index[i] += end_index[i - 1];
    }

    // Copy end_index to start_index
    std::copy_n(end_index, hash_table_size, start_index);

    // For every particle, subtract -1 at the hash index and add the index to particleMap
    for (int i = 0; i < num_points; i++)
    {
        glm::ivec3 cellId = grid->getCellId(positions[i]);
        int hashedCellId = hashCoords(cellId);
        start_index[hashedCellId]--;
        particleMap[start_index[hashedCellId]] = i;
    }
}

Particles::~Particles()
{
    delete[] positions;
    delete[] colors;
    delete[] velocities;
    delete[] densities;
    delete[] pressures;
    delete[] forces;
    delete[] start_index;
    delete[] end_index;
    delete[] particleMap;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}