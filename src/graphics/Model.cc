#include "Model.h"
namespace engine {
    Mesh::Mesh(const data::MeshData& meshData, const data::MaterialData& material)
        : m_material(material) {
        SetupMesh(meshData);
    }
    
    void Mesh::Draw(Shader& shader) {
        // Биндим текстуры
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        
        for (unsigned int i = 0; i < m_material.textures.size(); i++) {
            const auto& texInfo = m_material.textures[i];
            texInfo.texture->Bind(i);
            
            std::string number;
            std::string name = "texture_";
            
            if (texInfo.type == "diffuse")
                number = std::to_string(diffuseNr++);
            else if (texInfo.type == "specular")
                number = std::to_string(specularNr++);
            else if (texInfo.type == "normal")
                number = std::to_string(normalNr++);
                
            shader.SetInt(("material." + name + number).c_str(), i);
        }
        
        glActiveTexture(GL_TEXTURE0);
        
        SetupMaterial(shader, m_material);
        m_vao->Bind();
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void Mesh::SetupMaterial(Shader& shader, const data::MaterialData& material) {
          glm::vec3 diffuse = (material.diffuseColor == glm::vec3(0.0f)) 
        ? glm::vec3(0.8f) : material.diffuseColor;
        shader.SetVec3("material.diffuse", material.diffuseColor);
        shader.SetVec3("material.specular", material.specularColor);
        shader.SetFloat("material.shininess", material.shininess);
    }
    
    void Mesh::SetupMesh(const data::MeshData& meshData) {
        m_vao = VertexArrayObject::Create();
        
        m_vbo = VertexBuffer::Create(
            meshData.vertices.size() * sizeof(data::Vertex)
        );
        m_vbo->SetData(meshData.vertices.data(), meshData.vertices.size() * sizeof(data::Vertex));

        m_vbo->SetLayout({
            {ShaderDataType::Float3, "a_position"},
            {ShaderDataType::Float3, "a_normal"}, 
            {ShaderDataType::Float2, "a_texCoords"},
            {ShaderDataType::Float3, "a_tangent"},
            {ShaderDataType::Float3, "a_bitangent"}
        });

        m_vao->AddVertexBuffer(m_vbo);

        m_ebo = IndexBuffer::Create(
            const_cast<uint32_t*>(meshData.indices.data()),
            meshData.indices.size()
        );

        m_vao->SetIndexBuffer(m_ebo);
        m_indexCount = meshData.indices.size();
    }
    void Model::Draw(Shader& shader)
    {
        for (auto& mesh : m_meshes) {
            mesh.Draw(shader);
        }
    }
    void Model::SetupModel()
    {
        for (size_t i = 0; i < m_modelData.meshes.size(); i++) {
            const auto& meshData = m_modelData.meshes[i];
            
            const data::MaterialData* material = nullptr;
            if (meshData.materialIndex < m_modelData.materials.size()) {
                material = &m_modelData.materials[meshData.materialIndex];
            } else {
                static data::MaterialData defaultMaterial;
                material = &defaultMaterial;
            }
            
            data::MaterialData materialWithTextures = *material;
            for (auto& texInfo : materialWithTextures.textures) {
                bool found = false;
                for (const auto& cachedTex : m_texturesLoaded) {
                    if (cachedTex.path == texInfo.path) {
                        texInfo.texture = cachedTex.texture;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    texInfo.texture = Texture::Create(texInfo.path);
                    m_texturesLoaded.push_back(texInfo);
                }
            }
            
            m_meshes.emplace_back(meshData, materialWithTextures);
        }
    }
}