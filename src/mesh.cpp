#include <mesh.hpp>
#include <glad/glad.h>
#ifndef TINYOBJLOADER_IMPLEMENTATION
    #define TINYOBJLOADER_IMPLEMENTATION
#endif
#include <tiny_obj_loader.h>
#include <iostream>

Mesh::Mesh(const char* path, Texture* const texture, Shader* const shader){
    this->path = path;
    this->texture = texture;
    this->shader = shader;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
    {
        throw std::runtime_error(warn + err);
    }

	tinyobj::index_t i0, i1, i2;
	for (size_t i = 0; i < shapes[0].mesh.indices.size(); i+=3) {
		// Get the vertices of the triangle
		i0 = shapes[0].mesh.indices[i];
		i1 = shapes[0].mesh.indices[i+1];
		i2 = shapes[0].mesh.indices[i+2];

		// Get the vertices of the triangle
		glm::vec3 v0(attrib.vertices[3*i0.vertex_index], attrib.vertices[3*i0.vertex_index+1], attrib.vertices[3*i0.vertex_index+2]);
		glm::vec3 v1(attrib.vertices[3*i1.vertex_index], attrib.vertices[3*i1.vertex_index+1], attrib.vertices[3*i1.vertex_index+2]);
		glm::vec3 v2(attrib.vertices[3*i2.vertex_index], attrib.vertices[3*i2.vertex_index+1], attrib.vertices[3*i2.vertex_index+2]);
		// Get the normals of the triangle
		glm::vec3 n0(attrib.normals[3*i0.normal_index], attrib.normals[3*i0.normal_index+1], attrib.normals[3*i0.normal_index+2]);
		glm::vec3 n1(attrib.normals[3*i1.normal_index], attrib.normals[3*i1.normal_index+1], attrib.normals[3*i1.normal_index+2]);
		glm::vec3 n2(attrib.normals[3*i2.normal_index], attrib.normals[3*i2.normal_index+1], attrib.normals[3*i2.normal_index+2]);
		// Get the uv coordinates of the triangle
		glm::vec2 uv0(attrib.texcoords[2*i0.texcoord_index], attrib.texcoords[2*i0.texcoord_index+1]);
		glm::vec2 uv1(attrib.texcoords[2*i1.texcoord_index], attrib.texcoords[2*i1.texcoord_index+1]);
		glm::vec2 uv2(attrib.texcoords[2*i2.texcoord_index], attrib.texcoords[2*i2.texcoord_index+1]);

        // Add the vertices to the mesh
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        
        // Add the normals to the mesh
        normals.push_back(n0);
        normals.push_back(n1);
        normals.push_back(n2);
        
        // Add the uv coordinates to the mesh
        uvs.push_back(uv0);
        uvs.push_back(uv1);
        uvs.push_back(uv2);

        // Add the indices to the mesh
        indices.push_back(i);
        indices.push_back(i+1);
        indices.push_back(i+2);
    }

    setupMesh();
}

void Mesh::setupMesh(){
    const GLsizei stride = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // vertex normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    // vertex texture coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2*sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::Draw(const glm::mat4 model)
{
    shader->use();
    shader->setMat4("model", model);
    texture->Bind();
    glBindVertexArray(VAO);
    // dont use ebos for now
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    // use ebos for now
    // glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}

void Mesh::Draw(){
    glm::mat4 model = glm::mat4(1.0f);
    Draw(model);
}

void Mesh::setTexture(Texture* const texture){
    this->texture = texture;
}

void Mesh::setShader(Shader* const shader){
    this->shader = shader;
}