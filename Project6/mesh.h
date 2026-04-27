#pragma once
#include <glad/glad.h>
#include <glm.hpp>
#include <vector>
#include "shader.h"
#include <cstddef>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    // При желании добавьте текстурные координаты: glm::vec2 TexCoords;
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
