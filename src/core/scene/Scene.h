#pragma once
#include "Components.h"
#include "graphics/Renderer.h"
#include "core/AgentManager.h"
#include "Camera.h"
#include "data/PrefabSystem.h"

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
struct SceneObject {
    int agent_id;           // ID агента в симуляции
    std::string model_path; // Модель для рендеринга
    glm::vec3 position;     // Позиция (дублируется с агентом)
    
    bool is_visible = true;
};

  std::vector<Agent> LoadAgentsFromGPU();
  void RenderAgent(const Agent& agent);

  AgentManager m_agentManager;
  Renderer m_renderer;
  std::unordered_map<std::string, std::shared_ptr<Model>> m_loadedModels;
  std::vector<SceneObject> m_sceneObjects;
  data::PrefabSystem m_prefabSystem;
  std::shared_ptr<Model> m_treeModel;
  std::shared_ptr<Model> m_burningModel;
  std::shared_ptr<Model> m_burnedModel;
  Camera m_camera;
};
} // namespace engine