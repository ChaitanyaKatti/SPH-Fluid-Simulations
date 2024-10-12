#include <glm/glm.hpp>
#include <spatialgrid.hpp>
#include <config.hpp>
#include <shader.hpp>
#include <iostream>
#include <vector>
#include <glad/glad.h>

SpatialGrid::SpatialGrid(glm::vec3 num_cells, glm::mat4 transform, Shader* const shader)
{
    this->transform = transform;
    this->inv_transform = glm::inverse(transform);
    this->shader = shader;

    // Create 12 edges of the cube
    this->vertices = {
        0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,    1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f,

        0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f
    };

    // Add lines to draw each cell
    for (int i=0; i<=num_cells.x; i++)
    {
        for (int j=0; j<=num_cells.y; j++)
        {
            float i_f = i / num_cells.x;
            float j_f = j / num_cells.y;

            vertices.push_back(i_f); vertices.push_back(j_f); vertices.push_back(0.0f);
            vertices.push_back(i_f); vertices.push_back(j_f); vertices.push_back(1.0f);

            vertices.push_back(i_f); vertices.push_back(0.0f); vertices.push_back(j_f);
            vertices.push_back(i_f); vertices.push_back(1.0f); vertices.push_back(j_f);

            vertices.push_back(0.0f); vertices.push_back(i_f); vertices.push_back(j_f);
            vertices.push_back(1.0f); vertices.push_back(i_f); vertices.push_back(j_f);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SpatialGrid::transformPositionVelocity(glm::vec3 &p, glm::vec3 &v)
{
    p = glm::vec3(inv_transform * glm::vec4(p, 1));
    v = glm::vec3(inv_transform * glm::vec4(v, 0));
    
    if (p.y < 0.0f)
    {
        p.y = EPSILON;
        v.y *= -COEFF_RESTITUTION;
    }
    else if (p.y > 1.0f)
    {
        p.y = 1.0f - EPSILON;
        v.y *= -COEFF_RESTITUTION;
    }
    if (p.x < 0.0f)
    {
        p.x = EPSILON;
        v.x *= -COEFF_RESTITUTION;
    }
    else if (p.x > 1.0f)
    {
        p.x = 1.0f - EPSILON;
        v.x *= -COEFF_RESTITUTION;
    }
    if (p.z < 0.0f)
    {
        p.z = EPSILON;
        v.z *= -COEFF_RESTITUTION;
    }
    else if (p.z > 1.0f)
    {
        p.z = 1.0f - EPSILON;
        v.z *= -COEFF_RESTITUTION;
    }

    p = glm::vec3(transform * glm::vec4(p, 1));
    v = glm::vec3(transform * glm::vec4(v, 0));

    return;
}

void SpatialGrid::drawGrid()
{
    glBindVertexArray(VAO);
    shader->use();
    shader->setMat4("modelMatrix", transform);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
    glBindVertexArray(0);
}

void SpatialGrid::drawBoundary()
{
    glBindVertexArray(VAO);
    shader->use();
    shader->setMat4("modelMatrix", transform);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}

void SpatialGrid::setTransform(glm::mat4 transform)
{
    this->transform = transform;
    this->inv_transform = glm::inverse(transform);
}

glm::mat4 SpatialGrid::getTransform()
{
    return transform;
}

