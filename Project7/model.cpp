#include "model.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <gtc/type_ptr.hpp>

Model::Model(const char* path) {
    loadModel(path);
    meshTransforms.resize(meshes.size(), glm::mat4(1.0f));
}

void Model::Draw(Shader& shader) {
    for (unsigned int i = 0; i < meshes.size(); ++i) {
        // Применяем анимационную матрицу поверх исходной трансформации узла
        shader.setUniform("model", meshTransforms[i]);
        meshes[i].Draw(shader);
    }
}

void Model::setMeshTransform(const std::string& meshName, const glm::mat4& transform) {
    auto it = nameToIndex.find(meshName);
    if (it != nameToIndex.end())
        meshTransforms[it->second] = transform;
    else
        std::cerr << "Mesh not found: " << meshName << std::endl;
}

unsigned int Model::getMeshCount() const {
    return (unsigned int)meshes.size();
}

const std::vector<std::string>& Model::getMeshNames() const {
    return meshNames;
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene, glm::mat4(1.0f));
}

void Model::processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
    // Преобразование матрицы узла из Assimp (row-major) в glm (column-major)
    aiMatrix4x4 aiMat = node->mTransformation;
    glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&aiMat.a1));

    // Итоговая трансформация для всех мешей этого узла
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // Обрабатываем все меши данного узла
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, globalTransform));

        std::string meshName = mesh->mName.C_Str();
        if (meshName.empty())
            meshName = "Mesh_" + std::to_string(meshes.size() - 1);
        meshNames.push_back(meshName);
        nameToIndex[meshName] = (int)meshes.size() - 1;
    }

    // Рекурсивный обход дочерних узлов
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene, globalTransform);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;
        // Применяем трансформацию к позиции
        glm::vec4 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        vertex.Position = transform * pos;

        // Применяем трансформацию к нормалям (без сдвига)
        if (mesh->HasNormals()) {
            glm::vec4 norm(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
            vertex.Normal = glm::normalize(glm::vec3(transform * norm));
        }
        else {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    return Mesh(vertices, indices);
}