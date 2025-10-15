#include "Material.h"
#include "graphics/Texture.h"
#include <exception>
#include <iostream>

namespace engine {
std::shared_ptr<Material> Material::Create(const std::string& diffusePath,
        const std::string& specularPath, 
        const std::string& normalPath,
        const glm::vec3& diffuseColor,
        const glm::vec3& specularColor,
        float shininess) {
            auto  material = std::make_shared<Material>();
            if(!diffusePath.empty()) {
                try {
                    material->m_diffuseTexture = Texture::Create(diffusePath);
                } catch(const std::exception& e) {
                    std::cout << "Failed to load diffuse texture: " << e.what() << std::endl;
                }
            }

            if(!specularPath.empty()) {
                try {
                    material->m_specularTexture = Texture::Create(specularPath);
                } catch(const std::exception& e) {
                    std::cout << "Failed to load specular texture: " << e.what() << std::endl;
                }
            }

            if(!normalPath.empty()) {
                try {
                    material->m_normalTexture = Texture::Create(normalPath);
                } catch (const std::exception& e) {
                    std::cout << "Failed to load normal texture" << e.what() << std::endl;
                }
            }
            std::cout << "Diffuse color create: " << diffuseColor.r << ", " << diffuseColor.g << ", " << diffuseColor.b << std::endl;

            material->m_diffuseColor = diffuseColor;
            material->m_specularColor = specularColor;
            return material;
        }

void Material::Bind(Shader& shader) const {
    if (m_diffuseTexture) {
        // m_diffuseTexture->Bind(0);
        // std::cout << "here" << std::endl;
        // shader.SetInt("material.diffuse_map", 0);
        // shader.SetBool("material.use_diffuse_map", true);
        shader.SetVec3("material.diffuse", m_diffuseColor);
    } else {
        shader.SetBool("material.use_diffuse_map", false);
        shader.SetVec3("material.diffuse", m_diffuseColor);
    }
    
    if (m_specularTexture) {
        // m_specularTexture->Bind(1);
        // shader.SetInt("material.specular_map", 1);
        // shader.SetBool("material.use_specular_map", true);
        shader.SetVec3("material.specular", m_specularColor);

    } else {
        shader.SetBool("material.use_specular_map", false);
        shader.SetVec3("material.specular", m_specularColor);
    }
    
    if (m_normalTexture) {
        m_normalTexture->Bind(2);
        shader.SetInt("material.normal_map", 2);
        shader.SetBool("material.use_normal_map", true);
    } else {
        shader.SetBool("material.use_normal_map", false);
    }
    
    shader.SetFloat("material.shininess", m_shininess);
}

} // namespace engine