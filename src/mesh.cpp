#include <mesh.hpp>
#include <glad/glad.h>
#ifndef TINYOBJLOADER_IMPLEMENTATION
    #define TINYOBJLOADER_IMPLEMENTATION
#endif
#include <tiny_obj_loader.h>
#include <iostream>

Mesh::Mesh(const char* path, Texture* const texture, Shader* const shader) {
    this->path = path;
    this->texture = texture;
    this->shader = shader;
    this->model = glm::mat4(1.0f);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
    {
        throw std::runtime_error(warn + err);
    }

    tinyobj::index_t idx;
    for (size_t i = 0; i < shapes[0].mesh.indices.size(); ++i) {
        idx = shapes[0].mesh.indices[i];
        // Retrieve vertex data using index
        glm::vec3 vertex(attrib.vertices[3 * idx.vertex_index],
                         attrib.vertices[3 * idx.vertex_index + 1],
                         attrib.vertices[3 * idx.vertex_index + 2]);
        glm::vec3 normal(attrib.normals[3 * idx.normal_index],
                         attrib.normals[3 * idx.normal_index + 1],
                         attrib.normals[3 * idx.normal_index + 2]);
        glm::vec2 uv(attrib.texcoords[2 * idx.texcoord_index],
                     attrib.texcoords[2 * idx.texcoord_index + 1]);

        // Add to respective vectors
        positions.push_back(vertex);
        normals.push_back(normal);
        uvs.push_back(uv);

        // Add to vertices vector
        vertices.push_back(vertex.x);
        vertices.push_back(vertex.y);
        vertices.push_back(vertex.z);
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
        vertices.push_back(uv.x);
        vertices.push_back(uv.y);
    }

    setupMesh();
}

void Mesh::setupMesh() {
    const GLsizei stride = 8 * sizeof(float);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // vertex normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // vertex texture coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::Draw(const glm::mat4 model)
{
    shader->use();
    shader->setMat4("modelMatrix", model);
    shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    if (texture)
    {
        texture->Bind();
    }
    else{
        shader->setVec3("color", glm::vec3(1.0f));
    }

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 8);
    glBindVertexArray(0); // Unbind VAO
}

void Mesh::Draw(){
    Draw(this->model);
}

void Mesh::SetInstances(int num_instances, const glm::vec3* offsets){
    this->offsets = std::vector<glm::vec3>(offsets, offsets + num_instances);
    this->num_instances = num_instances;
    // Generate buffer for offsets
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, num_instances * sizeof(glm::vec3), offsets, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Bind VAO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(3, 1);
    glBindVertexArray(0);
}

void Mesh::DrawInstances(const glm::mat4 model){
    shader->use();
    shader->setMat4("modelMatrix", model);
    shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    if (texture)
    {
        texture->Bind();
    }
    else{
        shader->setVec3("color", glm::vec3(1.0f));
    }

    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size() / 8, this->num_instances);
    glBindVertexArray(0); // Unbind VAO
}

void Mesh::setTexture(Texture* const texture){
    this->texture = texture;
}

void Mesh::setShader(Shader* const shader){
    this->shader = shader;
}

void Mesh::setModel(const glm::mat4 model){
    this->model = model;
}

glm::mat4 Mesh::getModel(){
    return model;
}