#include "Scene.h"
#include <cuda_runtime.h>
#include <glm/gtc/matrix_transform.hpp>  // glm::perspective
#include "Entity.h"
#include "core/Input.h"
#include "core/simulation.cuh"
namespace engine {
Scene::Scene() : m_agentManager(1000000), m_camera(glm::vec3(500.0f, 100.0f, 600.0f)) {}

void Scene::Init() {
  LoadObjectModels("default_tree", "res/models/LowPolytree/CartoonTree.obj",
                   "res/models/CartoonTree/CartoonTree.obj", "res/models/ChrTree/CartoonTree.obj");
  LoadObjectModels("default_house", "res/models/house/house.obj", "res/models/house/house.obj",
                   "res/models/house/house.obj");
  m_renderer.Init();

  m_prefabSystem.LoadPrefabFromFile("res/prefabs/default_house.yaml");

  m_prefabSystem.LoadPrefabFromFile("res/prefabs/default_tree.yaml");
}

void Scene::Update(float dt) {
  if (!m_sceneObjects.empty()) {
    SimulationStep(m_agentManager.getDeviceAgents(), m_agentManager.getAgentCount(), dt);
  }

  const bool ui_mouse = engine::InputSystem::Instance().UIMouseCaptured();
  const bool ui_kbd = engine::InputSystem::Instance().UIKeyboardCaptured();

  // 2) WASD — только если UI не держит клавиатуру
  if (!ui_kbd) {
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::W))
      m_camera.ProcessKeyboard(FORWARD, dt);
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::S))
      m_camera.ProcessKeyboard(BACKWARD, dt);
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::D))
      m_camera.ProcessKeyboard(RIGHT, dt);
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::A))
      m_camera.ProcessKeyboard(LEFT, dt);
  }

  // 3) Поворот камеры — только если UI не держит мышь И зажат RMB
  const glm::vec2 delta = engine::InputSystem::Instance().GetMouseDelta();
  if (!ui_mouse && engine::InputSystem::Instance().GetMouseButton(engine::MouseButton::Right,
                                                                  InputState::Held)) {
    if (delta.x != 0.f || delta.y != 0.f) {
      m_camera.ProcessMouseMovement(delta.x, delta.y);
    }
  }

  // 5) Зум — только если UI не держит мышь/скролл
  if (!ui_mouse) {
    const float scrollY = engine::InputSystem::Instance().GetMouseScrollY();
    if (scrollY != 0.0f) {
      m_camera.ProcessMouseScroll(scrollY);
      engine::InputSystem::Instance().ResetMouseScroll();
    }
  }
}

void Scene::Render() {
  const int fbW = int(m_vpW * m_vpScaleX);
  const int fbH = int(m_vpH * m_vpScaleY);
  glViewport(0, 0, fbW, fbH);
  auto projection =
      glm::perspective(m_camera.GetZoom(), fbH > 0 ? float(fbW) / float(fbH) : 1.0f, 0.1f, 2000.0f);

  auto view = m_camera.GetViewMatrix();
  m_renderer.BeginScene(view, projection);

  auto agents = LoadAgentsFromGPU();

  for (const auto& obj : m_sceneObjects) {
    if (!obj.is_visible) {
      continue;
    }

    if (obj.agent_id < agents.size()) {
      const Agent& agent = agents[obj.agent_id];
      RenderAgent(agent, obj);
    }
  }

  std::vector<Agent> burningBranches;
  for (const auto& agent : agents) {
    if ((agent.type == AgentType::TREE || agent.type == AgentType::BURNED)) {
      burningBranches.push_back(agent);
    }
  }
  m_renderer.UploadAgentsData(agents);
  m_renderer.RenderWithGeometryShader();

  m_renderer.EndScene();
}

void Scene::AddTree(float x, float y, const std::string& tree_type) {
  const auto& prefab = m_prefabSystem.GetPrefab(tree_type);
  Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, 0, y});
  m_agentManager.AddAgent(agent);

  SceneObject obj;
  obj.agent_id = m_agentManager.getAgentCount() - 1;
  obj.object_type = tree_type;
  obj.position = glm::vec3(x, 0, y);
  obj.is_visible = true;
  obj.scale = prefab.model_scale;
  m_sceneObjects.push_back(obj);
}

void Scene::Generate() {
  for (int i = 0; i < 100; i += 3) {
    for (int j = 0; j < 100; j += 3) {
      AddTree(500 + i, 500 + j, "default_tree");
    }
  }
}

std::vector<Agent> Scene::LoadAgentsFromGPU() {
  std::vector<Agent> agents(m_agentManager.getAgentCount());
  cudaMemcpy(agents.data(), m_agentManager.getDeviceAgents(), agents.size() * sizeof(Agent),
             cudaMemcpyDeviceToHost);
  return agents;
}

void Scene::RenderAgent(const Agent& agent, const SceneObject& scene_obj) {
  glm::vec3 position(agent.position.x, agent.position.y, agent.position.z);

  ObjectModels* models = nullptr;
  auto it = m_objectModels.find(scene_obj.object_type);
  if (it != m_objectModels.end()) {
    models = &it->second;
  }

  std::shared_ptr<Model> model_to_render;

  if (agent.type == AgentType::BURNED) {
    // Сгоревшее состояние
    model_to_render = models->burned;
  } else if (agent.isBurning() || agent.type == AgentType::FIRE_SOURCE) {
    // Горящее состояние
    model_to_render = models->burning;
  } else {
    // Нормальное состояние
    model_to_render = models->normal;
  }
  if (model_to_render)
    m_renderer.DrawModel(*model_to_render, position, scene_obj.scale, m_camera.GetPosition());
}

glm::mat4 Scene::GetProjMatrix(int w, int h) {
  return glm::perspective(m_camera.GetZoom(), static_cast<float>(w) / static_cast<float>(h), 0.1f,
                          2000.0f);
}

void Scene::Clear() {
  m_sceneObjects.clear();
  m_agentManager.Clear();
}


void Scene::AddPrefab(const std::string& prefab_id, float x, float z) {
  const auto& prefab = m_prefabSystem.GetPrefab(prefab_id);
  Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, 0, z});
  m_agentManager.AddAgent(agent);

  SceneObject obj;
  obj.agent_id   = m_agentManager.getAgentCount() - 1;
  obj.object_type= prefab_id;                // ключ для выбора модели при рендере
  obj.position   = glm::vec3(x, 0, z);
  obj.scale      = prefab.model_scale;
  obj.is_visible = true;
  m_sceneObjects.push_back(obj);
}

// Если нужно, оставь AddTree как шорткат:
void Scene::AddTree(float x, float z, const std::string& tree_type) {
  AddPrefab(tree_type, x, z);
}

// Список id префабов (проксируем)
const std::vector<std::string>& Scene::ListPrefabIds() const {
  return m_prefabSystem.ListIds();
}


}  // namespace engine
