#include <glad/glad.h>
#include <iostream>

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

Particles::Particles(const float mass, const float resting_density, const float radius, glm::vec3 *positions, glm::vec3 *colors, int num_points, Shader *const shader) : mass(mass), resting_density(resting_density), radius(radius), positions(positions), colors(colors), num_points(num_points), shader(shader)
{
    countArray = new int[hashTableSize];
    startIndex = new int[hashTableSize];
    stopIndex = new int[hashTableSize];
    indexArray = new int[num_points];

    setupParticles();
}

void Particles::setupParticles()
{
    this->velocities = new glm::vec3[num_points];
    for (int i = 0; i < num_points; i++)
    {
        velocities[i] = glm::vec3(0.0f);
    }
    this->densities = new float[num_points];
    this->pressures = new float[num_points];
    this->forces = new glm::vec3[num_points];

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
    std::cout << "Updating hash" << "\n";
    updateHash();
    std::cout << "Calculating pressure" << "\n";
    calculateDensityAndPressure();
    std::cout << "Applying forces asdasd" << "\n";
    applyForces(dt);
    // std::cout << "positions " << positions[0].x << " " << positions[0].y << " " << positions[0].z << std::endl;
    // std::cout << "forces    "  << forces[0].x << " " << forces[0].y << " " << forces[0].z << std::endl;
    // Update VBO

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, num_points * sizeof(glm::vec3), num_points * sizeof(glm::vec3), colors);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

inline int Particles::hash(glm::vec3 p){
    int hash = (int(p.x/h) * 92837111) ^ (int(p.y/h) * 6892287499) ^ (int(p.z/h) * 283923481);
    return abs(hash) % hashTableSize;
}

void Particles::updateHash(){

    // Initialize all arrays to zero
    std::fill(countArray, countArray + 1000, 0);
    std::fill(startIndex, startIndex + 1000, 0);
    std::fill(stopIndex, stopIndex + 1000, 0);
    std::fill(indexArray, indexArray + num_points, 0);

    // For each particle, find its cell Id and add to count array
// #pragma omp parallel for shared(countArray)
    for (int i = 0; i < num_points; i++)
    {
        int cellId = hash(positions[i]); //(int)(positions[i].x) + (int)(positions[i].y) * 10 + (int)(positions[i].z) * 100;
        // cellId = cellId % 1000;
        countArray[cellId]++;
    }

    // Do partial sum and store in startIndex and stopIndex
    stopIndex[0] = countArray[0];
    for (int i = 1; i < 1000; i++)
    {
        stopIndex[i] = stopIndex[i - 1] + countArray[i];
    }
    // copy to start index
    std::copy_n(stopIndex, 1000, startIndex);

    // For each particle, find its cell Id and add 
// #pragma omp parallel for shared(startIndex)
    for (int i=0; i<num_points; i++){
        int cellId = hash(positions[i]); //(int)(positions[i].x) + (int)(positions[i].y) * 10 + (int)(positions[i].z) * 100;
        // cellId = cellId % 1000;
        startIndex[cellId]--;
        indexArray[startIndex[cellId]] = i;
    }
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
#pragma omp parallel for
    for (int i = 0; i < num_points; i++)
    {
        densities[i] = 0.0f;
        // for (int j = 0; j < num_points; j++)
        // {
        //     if (i != j)
        //     {
        //         glm::vec3 r = positions[i] - positions[j];
        //         float r2 = glm::dot(r, r);
        //         densities[i] += mass * poly6Kernel(r2);
        //     }
        // }

        for (int x = -1; x <= 1; x++)
        {
            if ((positions[i].x < 1 && x==-1) || (positions[i].x > 9 && x==1))
                continue;
            for (int y = -1; y <= 1; y++)
            {
                if ((positions[i].y < 1 && y==-1) || (positions[i].y > 9 && y==1))
                    continue;
                for (int z = -1; z <= 1; z++)
                {   
                    if ((positions[i].z < 1 && z==-1) || (positions[i].z > 9 && z==1))
                        continue;

                    int cellId = hash(positions[i] + glm::vec3(x ,y, z)); //((int)(positions[i].x) + x) + ((int)(positions[i].y) + y) * 10 + ((int)(positions[i].z) + z) * 100;
                    // cellId = cellId % 1000;
                    if( stopIndex[cellId] - startIndex[cellId]>1){
                    }
                    for (int j = startIndex[cellId]; j < stopIndex[cellId]; j++)
                    {
                        int neighborIndex = indexArray[j];
                        if (i != neighborIndex)
                        {
                            glm::vec3 r = positions[i] - positions[neighborIndex];
                            float r2 = glm::dot(r, r);
                            densities[i] += mass * poly6Kernel(r2);
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
#pragma omp parallel for
    for (int i = 0; i < num_points; i++)
    {
        forces[i] = glm::vec3(0.0f, -mass * g, 0.0f); // Gravity

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

        for (int x = -1; x <= 1; x++)
        {
            if ((positions[i].x < 1 && x == -1) || (positions[i].x > 9 && x == 1))
            continue;
            for (int y = -1; y <= 1; y++)
            {
            if ((positions[i].y < 1 && y == -1) || (positions[i].y > 9 && y == 1))
                continue;
            for (int z = -1; z <= 1; z++)
            {
                if ((positions[i].z < 1 && z == -1) || (positions[i].z > 9 && z == 1))
                continue;

                int cellId = hash(positions[i] + glm::vec3(x, y, z)); //((int)(positions[i].x) + x) + ((int)(positions[i].y) + y) * 10 + ((int)(positions[i].z) + z) * 100;
                // cellId = cellId % 1000;

                for (int j = startIndex[cellId]; j < stopIndex[cellId]; j++)
                {
                int neighborIndex = indexArray[j];
                if (i != neighborIndex)
                {
                    glm::vec3 r = positions[i] - positions[neighborIndex];
                    float sqrt_r = glm::length(r);
                    forces[i] += -mass * (pressures[i] + pressures[neighborIndex]) / (2.0f * densities[neighborIndex] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);  // Pressure term
                    forces[i] += mu * mass * (velocities[neighborIndex] - velocities[i]) / (densities[neighborIndex] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
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
        positions[i] += dt * velocities[i]; // + 0.5f * dt * dt * forces[i] / (densities[i] + DIVISON_EPSILON);

        // Apply boundary conditions
        if (positions[i].y < 0.0f)
        {
            positions[i].y = EPSILON;
            velocities[i].y *= -COEFF_RESTITUTION;
        }
        else if (positions[i].y > 10.0f)
        {
            positions[i].y = 10.0f - EPSILON;
            velocities[i].y *= -COEFF_RESTITUTION;
        }
        if (positions[i].x < 0.0f)
        {
            positions[i].x = EPSILON;
            velocities[i].x *= -COEFF_RESTITUTION;
        }
        else if (positions[i].x > 10.0f)
        {
            positions[i].x = 10.0f - EPSILON;
            velocities[i].x *= -COEFF_RESTITUTION;
        }
        if (positions[i].z < 0.0f)
        {
            positions[i].z = EPSILON;
            velocities[i].z *= -COEFF_RESTITUTION;
        }
        else if (positions[i].z > 10.0f)
        {
            positions[i].z = 10.0f - EPSILON;
            velocities[i].z *= -COEFF_RESTITUTION;
        }

        // Update colors
        float speed = glm::length(velocities[i]) / 5.0f;
        colors[i] = colorParticleBGR1(speed);
    }
}

void Particles::setPositions(glm::vec3 *positions)
{
    this->positions = positions;
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