#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm.hpp>
#include "shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    void Draw(Shader& shader);
private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
};