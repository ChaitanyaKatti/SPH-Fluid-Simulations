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
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;

    void setupMesh();
public:
    Mesh(const char* path, Texture* const texture, Shader* const shader);
    void Draw(const glm::mat4 model);
    void Draw();
    void setTexture(Texture* const texture);
    void setShader(Shader* const shader);
};