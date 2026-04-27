#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <assimp/scene.h>
#include "mesh.h"
#include "shader.h"

class Model {
public:
    Model(const char* path);
    void Draw(Shader& shader);
    void setMeshTransform(const std::string& meshName, const glm::mat4& transform);
    unsigned int getMeshCount() const;
    const std::vector<std::string>& getMeshNames() const;

private:
    std::vector<Mesh> meshes;
    std::vector<std::string> meshNames;
    std::unordered_map<std::string, int> nameToIndex;
    std::vector<glm::mat4> meshTransforms;  // индивидуальные дополнительные трансформации
    std::string directory;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform = glm::mat4(1.0f));
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
};