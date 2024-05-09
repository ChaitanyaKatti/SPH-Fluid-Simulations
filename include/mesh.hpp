#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <shader.hpp>
#include <texture.hpp>

class Mesh
{
private:
    unsigned int VAO, VBO, EBO;
    std::string path;
    Texture* texture;
    Shader* shader;
    std::vector<unsigned int> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<float> vertices;
    glm::mat4 model;
    int num_instances; // number of instances
    std::vector<glm::vec3> offsets; // for instanced rendering
    void setupMesh();
public:
    Mesh(const char* path, Texture* const texture, Shader* const shader);
    void Draw();
    void Draw(const glm::mat4 model);
    void SetInstances(int num_instances, const glm::vec3* offset);
    void DrawInstances(const glm::mat4 model);
    void setTexture(Texture* const texture);
    void setShader(Shader* const shader);
    void setModel(const glm::mat4 model);
    glm::mat4 getModel();
};


class Points
{
private:
    unsigned int VAO, VBO;
    Shader* shader;
    glm::vec3 *positions;
    int num_points;
    glm::mat4 modelMatrix;
    void setupPoints();
public:
    Points(glm::vec3* const positions, int num_points, Shader* const shader); 
    void Draw();
    void Draw(const glm::mat4 modelMatrix);
    void setShader(Shader* const shader);
    void setModel(const glm::mat4 model);
};