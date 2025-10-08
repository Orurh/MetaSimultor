#pragma once
#include "Components.h"
#include "graphics/Renderer.h"
#include "core/AgentManager.h"
#include "Camera.h"
namespace engine {
class Entity;
class Scene {
public:
  Scene();
  void Init();
  void Update(float dt);
  void Render();
  void AddTree(float x, float y);
  void AddFire(float x, float y) { m_agentManager.AddFire(x, y, 1.0f); }
  void Generate();
private:
  std::vector<Agent> LoadAgentsFromGPU();
  void RenderAgent(const Agent& agent);

  AgentManager m_agentManager;
  Renderer m_renderer;
  std::shared_ptr<Model> m_treeModel;
  std::shared_ptr<Model> m_burningModel;
  std::shared_ptr<Model> m_burnedModel;
  Camera m_camera;
};
} // namespace engine