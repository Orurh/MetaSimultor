#pragma once
#include "Components.h"
#include "graphics/Renderer.h"
#include "core/AgentManager.h"

namespace engine {
class Entity;
class Scene {
public:
  Scene();
  void Init();
  void Update(float dt);
  void Render();
  void AddTree(float x, float y) { m_agentManager.AddTree(x, y, 100.0f); }
  void AddFire(float x, float y) { m_agentManager.AddFire(x, y, 1.0f); }
  void SetViewProjection(const glm::mat4 &view, const glm::mat4& proj);

private:
  AgentManager m_agentManager;
  Renderer m_renderer;
  glm::mat4 m_view;
  glm::mat4 m_projection;

};
} // namespace engine