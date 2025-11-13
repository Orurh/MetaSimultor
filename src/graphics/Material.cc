#include "Material.h"
#include "TextureCache.h"
namespace engine {
std::shared_ptr<Material> Material::Create(const std::string &diffusePath,
                                           const std::string &specularPath,
                                           const std::string &normalPath,
                                           const glm::vec3 &diffuseColor,
                                           const glm::vec3 &specularColor,
                                           float shininess) {
  auto material = std::make_shared<Material>();
  material->m_diffuseColor = diffuseColor;
  material->m_specularColor = specularColor;
  material->m_shininess = shininess;

  auto &textureCache = TextureCache::Instance();

  if (!diffusePath.empty()) {
    material->m_diffuseTexture = textureCache.GetOrLoad(diffusePath);
  }
  if (!specularPath.empty()) {
    material->m_specularTexture = textureCache.GetOrLoad(specularPath);
  }
  if (!normalPath.empty()) {
    material->m_normalTexture = textureCache.GetOrLoad(normalPath);
  }

  return material;
}

void Material::Bind(Shader &shader) const {
  shader.SetVec3("material.diffuse", m_diffuseColor);
  shader.SetVec3("material.specular", m_specularColor);
  shader.SetBool("material.use_diffuse_map", false);
  shader.SetBool("material.use_normal_map", false);
  shader.SetBool("material.use_specular_map", false);

  shader.SetFloat("material.shininess", m_shininess);
}

} // namespace engine