#include <glm/glm.hpp>
#include <spatialgrid.hpp>
#include <config.hpp>
#include <shader.hpp>
#include <iostream>
#include <vector>
#include <glad/glad.h>

SpatialGrid::SpatialGrid(float cell_spacing, glm::mat4 transform, Shader *const shader) : cell_spacing(cell_spacing), transform(transform), shader(shader)
{
    // Get the x, y, z scalling from transform matrix
    float x_scale = glm::length(glm::vec3(transform[0]));
    float y_scale = glm::length(glm::vec3(transform[1]));
    float z_scale = glm::length(glm::vec3(transform[2]));
    this->num_cells = glm::ivec3(x_scale / cell_spacing, y_scale / cell_spacing, z_scale / cell_spacing);
    std::cout << "Num cells: " << num_cells.x << " " << num_cells.y << " " << num_cells.z << std::endl;
    std::cout << "Cell spacing: " << cell_spacing << std::endl;
    this->inv_transform = glm::inverse(transform);

    // Create 12 edges of the cube
    this->vertices = {
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f};

    // Add lines to draw each cell
    for (int i = 0; i <= num_cells.x; i++)
    {
        for (int j = 0; j <= num_cells.y; j++)
        {
            float i_f = i / num_cells.x;
            float j_f = j / num_cells.y;

            vertices.push_back(i_f);
            vertices.push_back(j_f);
            vertices.push_back(0.0f);
            vertices.push_back(i_f);
            vertices.push_back(j_f);
            vertices.push_back(1.0f);

            vertices.push_back(i_f);
            vertices.push_back(0.0f);
            vertices.push_back(j_f);
            vertices.push_back(i_f);
            vertices.push_back(1.0f);
            vertices.push_back(j_f);

            vertices.push_back(0.0f);
            vertices.push_back(i_f);
            vertices.push_back(j_f);
            vertices.push_back(1.0f);
            vertices.push_back(i_f);
            vertices.push_back(j_f);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
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

glm::mat4 SpatialGrid::getInvTransform()
{
    return inv_transform;
}

glm::ivec3 SpatialGrid::getNumCells()
{
    return num_cells;
}

float SpatialGrid::getCellSpacing()
{
    return cell_spacing;
}

glm::ivec3 SpatialGrid::getCellId(glm::vec3 p)
{
    glm::ivec3 cellId = p/cell_spacing;
    return cellId;
    // glm::vec3 p_transformed = glm::vec3(inv_transform * glm::vec4(p, 1));
    // glm::ivec3 cellId = p_transformed * glm::vec3(num_cells);
    // return cellId;
}

glm::ivec3 SpatialGrid::getCellIdWithOffset(glm::vec3 p, glm::ivec3 offset)
{
    glm::vec3 p_transformed = glm::vec3(inv_transform * glm::vec4(p, 1));
    glm::ivec3 cellId = p_transformed * glm::vec3(num_cells);
    cellId = glm::clamp(cellId + offset, glm::ivec3(0), num_cells - glm::ivec3(1));
    // cellId += offset;
    // if ((cellId.x < 0) || (cellId.y < 0) || (cellId.z < 0))
    //     return glm::ivec3(-1);
    // if ((cellId.x >= num_cells.x) || (cellId.y >= num_cells.y) || (cellId.z >= num_cells.z))
    //     return cellId;
    return cellId;
}

int SpatialGrid::getTotCells()
{
    return num_cells.x * num_cells.y * num_cells.z;
}