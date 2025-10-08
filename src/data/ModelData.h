#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <glm/glm.hpp>
#include "graphics/Texture.h"
namespace engine::data {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string materialName;
        uint32_t materialIndex;
    };

    struct TextureData {
        std::shared_ptr<engine::Texture> texture;  
        std::string type;                          
        std::string path;                          
        
        bool operator==(const TextureData& other) const {
            return path == other.path;
        }
    };

    struct MaterialData {
        std::string name;
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float shininess;
        std::vector<TextureData> textures;
    };

    struct ModelData {
        std::vector<MeshData> meshes;
        std::vector<MaterialData> materials;
        std::string directory;
        
        // Assimp processing flags
        static const unsigned int IMPORT_FLAGS = 
            aiProcess_Triangulate | 
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices;
    };

    class AssimpLoader {
    public:
        static ModelData LoadModel(const std::string& path);
        
    private:
        static void ProcessNode(aiNode* node, const aiScene* scene, ModelData& modelData);
        static MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
        static MaterialData ProcessMaterial(aiMaterial* material, const std::string& directory);
        static std::vector<TextureData> LoadMaterialTextures(
            aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::string& directory);
    };
}