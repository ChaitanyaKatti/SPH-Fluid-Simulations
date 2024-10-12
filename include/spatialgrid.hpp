#pragma once

#include <glm/glm.hpp>
#include <shader.hpp>

class SpatialGrid
{
private:
    glm::vec3 num_cells = glm::vec3(10.0f);
    glm::mat4 transform = glm::mat4(1.0f);
    glm::mat4 inv_transform = glm::mat4(1.0f);
    Shader* shader;

    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;

public:
    SpatialGrid(glm::vec3 num_cells, glm::mat4 transform, Shader* const shader);
    void transformPositionVelocity(glm::vec3 &p, glm::vec3 &v);
    void drawBoundary();
    void drawGrid();
    void setTransform(glm::mat4 transform);
    glm::mat4 getTransform();
};