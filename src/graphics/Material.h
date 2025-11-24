#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace engine {
class Texture;
class Shader;
class Material {
  public:
  Material() = default;
  static std::shared_ptr<Material> Create(const std::string &diffusePath = "", const std::string &specularPath = "",
                                          const std::string &normalPath = "",
                                          const glm::vec3 &diffuseColor = glm::vec3(0.8f),
                                          const glm::vec3 &specularColor = glm::vec3(0.5f), float shininess = 32.0f);
  // Установка текстур

  void SetDiffuseTexture(const std::shared_ptr<Texture> &texture) { m_diffuseTexture = texture; }
  void SetSpecularTexture(const std::shared_ptr<Texture> &texture) { m_specularTexture = texture; }
  void SetNormalTexture(const std::shared_ptr<Texture> &texture) { m_normalTexture = texture; }

  // Установка цветов
  void SetDiffuseColor(const glm::vec3 &color) { m_diffuseColor = color; }
  void SetSpecularColor(const glm::vec3 &color) { m_specularColor = color; }
  void SetShininess(float shininess) { m_shininess = shininess; }

  // Биндинг материала в шейдер
  void Bind(Shader &shader) const;
  void Release() {
    m_diffuseTexture.reset();
    m_specularTexture.reset();
    m_normalTexture.reset();
  }

  private:
  std::shared_ptr<Texture> m_diffuseTexture;
  std::shared_ptr<Texture> m_specularTexture;
  std::shared_ptr<Texture> m_normalTexture;

  glm::vec3 m_diffuseColor = glm::vec3(0.8f);
  glm::vec3 m_specularColor = glm::vec3(0.5f);
  float m_shininess = 32.0f;
};

}  // namespace engine