#pragma once

#include <glm/glm.hpp>
#include <shader.hpp>

class SpatialGrid
{
private:
    float cell_spacing;
    glm::mat4 transform;
    glm::mat4 inv_transform;
    Shader* shader;

    glm::ivec3 num_cells;

    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;

public:
    SpatialGrid(float cell_spacing, glm::mat4 transform, Shader* const shader);
    void transformPositionVelocity(glm::vec3 &p, glm::vec3 &v);
    void drawBoundary();
    void drawGrid();
    void setTransform(glm::mat4 transform);
    glm::mat4 getTransform();
    glm::mat4 getInvTransform();
    glm::ivec3 getNumCells();
    int getTotCells();
    glm::ivec3 getCellId(glm::vec3 p);
    glm::ivec3 getCellIdWithOffset(glm::vec3 p, glm::ivec3 offset);
    float getCellSpacing();
};