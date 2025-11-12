#include "ModelData.h"
#include <iostream>

namespace engine::data {
    ModelData AssimpLoader::LoadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, ModelData::IMPORT_FLAGS);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            throw std::runtime_error("Assimp error: " + std::string(importer.GetErrorString()));
        }
        
        ModelData modelData;
        modelData.directory = path.substr(0, path.find_last_of('/'));
        
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            modelData.materials.push_back(
                ProcessMaterial(scene->mMaterials[i], modelData.directory)
            );
        }
        
        ProcessNode(scene->mRootNode, scene, modelData);
        
        return modelData;
    }

    void AssimpLoader::ProcessNode(aiNode *node, const aiScene *scene, ModelData &modelData)
    {
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            modelData.meshes.push_back(ProcessMesh(mesh, scene));
        }
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene, modelData);
        }
    }

    MeshData AssimpLoader::ProcessMesh(aiMesh *mesh, const aiScene *scene)
    {
        MeshData meshData;
        meshData.materialIndex = mesh->mMaterialIndex;
        
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            // Позиции
            vertex.position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y, 
                mesh->mVertices[i].z
            );

            // Нормали
            if(mesh->HasNormals()) {
                vertex.normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            }

            // Текстурные координаты (берем первый набор)
            if(mesh->mTextureCoords[0]) {
                vertex.texCoords = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            } else {
                vertex.texCoords = glm::vec2(0.0f);
            }

            // Тангенты и битангенты (если есть)
            if(mesh->HasTangentsAndBitangents()) {
                vertex.tangent = glm::vec3(
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                );
                vertex.bitangent = glm::vec3(
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y, 
                    mesh->mBitangents[i].z
                );
            }

            meshData.vertices.push_back(vertex);
        }

        // Обрабатываем индексы
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                meshData.indices.push_back(face.mIndices[j]);
            }
        }

        return meshData;
    }
    MaterialData AssimpLoader::ProcessMaterial(aiMaterial* material, const std::string& directory) {
        MaterialData materialData;

        aiString name;
        if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            materialData.name = name.C_Str();
        }

        // Диффузный цвет
        aiColor3D diffuse(0.f, 0.f, 0.f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
            materialData.diffuseColor = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
        }

        // Спекулярный цвет
        aiColor3D specular(0.f, 0.f, 0.f);
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) == AI_SUCCESS) {
            materialData.specularColor = glm::vec3(specular.r, specular.g, specular.b);
        }

        // Блеск
        float shininess = 32.0f;
        if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            materialData.shininess = shininess;
        }

        auto diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", directory);
        materialData.textures.insert(materialData.textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", directory);
        materialData.textures.insert(materialData.textures.end(), specularMaps.begin(), specularMaps.end());

        auto normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", directory);
        materialData.textures.insert(materialData.textures.end(), normalMaps.begin(), normalMaps.end());
       
        return materialData;
    }

    std::vector<TextureData> AssimpLoader::LoadMaterialTextures(
        aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::string& directory) {
        
        std::vector<TextureData> textures;
        
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            TextureData textureData;
            textureData.type = typeName;
            textureData.path = directory + "/" + str.C_Str();

            textures.push_back(textureData);
        }

        return textures;
    }
}