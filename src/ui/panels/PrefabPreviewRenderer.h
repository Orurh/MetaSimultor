#pragma once
#include "data/PrefabSystem.h"
#include "graphics/Model.h"
#include <glm/glm.hpp>

namespace ui {

class PrefabPreviewRenderer {
public:
  PrefabPreviewRenderer(int width = 300, int height = 300);
  ~PrefabPreviewRenderer();

  void Render(const engine::data::AgentPrefab &prefab);

  GLuint GetTexture() const { return m_texture; }
  void Resize(int w, int h);

  void Rotate(float yaw, float pitch);
  void Zoom(float delta);
  void ResetView();

  

private:
  void UpdateCamera();
  void SetupFBO();

  GLuint m_fbo = 0, m_texture = 0, m_rbo = 0;
  int m_width, m_height;

  float m_yaw = -90.0f, m_pitch = 0.0f;
  float m_distance = 10.0f;
  const float m_minDist = 2.0f, m_maxDist = 50.0f;

  glm::vec3 m_target = glm::vec3(0);
  glm::mat4 m_view, m_proj;

  // Кэш
  std::shared_ptr<engine::Model> m_model;
  std::shared_ptr<engine::Material> m_material;
};

} // namespace ui