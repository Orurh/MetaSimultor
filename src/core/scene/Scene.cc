#include "Scene.h"
#include "Entity.h"
#include "core/Input.h"
#include "core/Window.h"
#include "core/agent.h"
#include "core/scene/Terrain.h"
#include "core/simulation.cuh"
#include <chrono>
#include <cuda_runtime.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <print>
namespace engine {
Scene::Scene()
    : m_agentManager(1000000), m_camera(glm::vec3(500.0F, 100.0F, 600.0F)) {}

void Scene::Init() {
  m_terrain = std::make_unique<Terrain>();
  TerrainGenerationSettings s;
  s.width = 256;
  s.depth = 256;
  s.scale = 2.0F;
  s.maxHeight = 40.0F;
  s.seed = 200;

  m_terrain->GenerateRandom(s);
  GetPrefabSystem().LoadAllFromDirectory("res/prefabs");
  m_renderer.Init();
  Generate();
}

void Scene::GenerateTerrain(const TerrainGenerationSettings &s) {
  m_terrain->GenerateRandom(s);
}

void Scene::EnsureModelsForPrefab(const std::string &id,
                                  const engine::data::AgentPrefab &prefab) {
  if (m_objectModels.contains(id)) {
    return;
  }

  const std::string &path = prefab.m_modelPath;
  if (path.empty()) {
    std::cerr << "[Scene] prefab '" << id << "' has empty model_path\n";
    return;
  }

  // Если у тебя нет отдельных путей для normal/burning/burned — грузим одну и
  // ту же модель.
  LoadObjectModels(id,
                   path, // normal
                   path, // burning
                   path  // burned
  );
}

void Scene::Update(float dt) {
  auto startTime = std::chrono::high_resolution_clock::now();

  m_windTime += dt;

  auto simStart = std::chrono::high_resolution_clock::now();
  if (!m_sceneObjects.empty()) {
    SimulationStep(m_agentManager.getDeviceAgents(),
                   m_agentManager.getAgentCount(), dt,
                   m_windConfig.getCurrentWind(m_windTime));
  }
  auto simTime = std::chrono::high_resolution_clock::now();
  //  std::cout << "sim time" <<
  //  std::chrono::duration_cast<std::chrono::milliseconds>(simTime -
  //  startTime).count() << std::endl;

  const bool UI_MOUSE = engine::InputSystem::Instance().UIMouseCaptured();
  const bool UI_KBD = engine::InputSystem::Instance().UIKeyboardCaptured();

  // 2) WASD — только если UI не держит клавиатуру
  if (!UI_KBD) {
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::W)) {
      m_camera.ProcessKeyboard(FORWARD, dt);
    }
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::S)) {
      m_camera.ProcessKeyboard(BACKWARD, dt);
    }
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::D)) {
      m_camera.ProcessKeyboard(RIGHT, dt);
    }
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::A)) {
      m_camera.ProcessKeyboard(LEFT, dt);
    }
  }

  // 3) Поворот камеры — только если UI не держит мышь И зажат RMB
  const glm::vec2 DELTA = engine::InputSystem::Instance().GetMouseDelta();
  if (!UI_MOUSE && engine::InputSystem::Instance().GetMouseButton(
                       engine::MouseButton::Right, InputState::Held)) {
    if (DELTA.x != 0.F || DELTA.y != 0.F) {
      m_camera.ProcessMouseMovement(DELTA.x, DELTA.y);
    }
  }

  // 5) Зум — только если UI не держит мышь/скролл
  if (!UI_MOUSE) {
    const float SCROLL_Y = engine::InputSystem::Instance().GetMouseScrollY();
    if (SCROLL_Y != 0.0F) {
      // m_camera.ProcessMouseScroll(SCROLL_Y);
      // engine::InputSystem::Instance().ResetMouseScroll();
    }
  }
}

void Scene::Render() {
  const int FB_W = static_cast<int>(m_vpW * m_vpScaleX);
  const int FB_H = static_cast<int>(m_vpH * m_vpScaleY);
  glViewport(0, 0, FB_W, FB_H);

  const float ASPECT =
      FB_H > 0 ? static_cast<float>(FB_W) / static_cast<float>(FB_H) : 1.0F;
  auto projection = glm::perspective(m_camera.GetZoom(), ASPECT, 0.1F, 2000.0F);

  auto view = m_camera.GetViewMatrix();
  m_renderer.BeginScene(view, projection);

  auto agents = LoadAgentsFromGPU();

  // Рендеринг объектов
  for (const auto &obj : m_sceneObjects) {
    if (!obj.is_visible) {
      continue;
    }

    if (obj.agent_id < agents.size()) {
      const Agent &agent = agents[obj.agent_id];
      RenderAgent(agent, obj);
    }
  }

  m_renderer.UploadAgentsData(agents);
  m_renderer.DrawTerrain(m_terrain->GetVAO(), m_camera.GetPosition());
  m_renderer.RenderWithGeometryShader(m_windConfig.getCurrentWind(m_windTime));

  m_renderer.EndScene();
}

void Scene::Generate() {
  for (int i = 0; i < 50; i += 3) {
    for (int j = 0; j < 50; j += 3) {
      AddTree(500 + i, 500 + j, "default_tree");
    }
  }
}

std::vector<Agent> Scene::LoadAgentsFromGPU() {
  size_t currentCount = m_agentManager.getAgentCount();
  std::vector<Agent> agents(currentCount);

  cudaMemcpy(agents.data(), m_agentManager.getDeviceAgents(),
             currentCount * sizeof(Agent), cudaMemcpyDeviceToHost);

  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::println("CUDA memcpy error: {}", cudaGetErrorString(err));
  }

  return agents;
}
void Scene::LoadObjectModels(const std::string &object_name,
                             const std::string &normal_path,
                             const std::string &burning_path,
                             const std::string &burned_path) {
  try {
    ObjectModels models;
    models.normal = std::make_shared<Model>(normal_path);
    models.burning = std::make_shared<Model>(burning_path);
    models.burned = std::make_shared<Model>(burned_path);
    m_objectModels[object_name] = models;
  } catch (const std::exception &e) {
    std::cout << "Failed to load model '" << object_name << "': " << e.what()
              << '\n';
  }
}

void Scene::RenderAgent(const Agent &agent, const SceneObject &scene_obj) {
  glm::vec3 position(agent.position.x, agent.position.y, agent.position.z);

  // Получаем префаб
  const auto &prefab = m_prefabSystem.GetPrefab(scene_obj.object_type);

  // Получаем модель
  auto it = m_objectModels.find(scene_obj.object_type);
  if (it == m_objectModels.end() || !it->second.normal) {
    std::cerr << "[RenderAgent] no model for '" << scene_obj.object_type
              << "'\n";
    return;
  }

  // Определяем состояние агента
  AgentType state = AgentType::NORMAL;
  if (agent.type == AgentType::BURNED) {
    state = AgentType::BURNED;
  } else if (agent.isBurning() || agent.type == AgentType::BURNING) {
    state = AgentType::BURNING;
  }

  auto material = prefab.GetMaterialForState(state);
  auto model = it->second.normal;

  if (model && material) {
    m_renderer.DrawModelWithMaterial(*model, position, scene_obj.scale,
                                     m_camera.GetPosition(), material);
  } else {
    // Fallback
    m_renderer.DrawModel(*model, position, scene_obj.scale,
                         m_camera.GetPosition());
  }
}

glm::mat4 Scene::GetProjMatrix(int w, int h) {
  return glm::perspective(m_camera.GetZoom(),
                          static_cast<float>(w) / static_cast<float>(h), 0.1F,
                          2000.0F);
}

void Scene::Clear() {
  m_sceneObjects.clear();
  m_agentManager.Clear();
}

void Scene::AddPrefab(const std::string &prefab_id, float x, float z) {
  const auto &prefab = m_prefabSystem.GetPrefab(prefab_id);

  // 1) гарантируем, что модели для этого prefab загружены
  EnsureModelsForPrefab(prefab_id, prefab);

  const float y = GetGroundHeight(x, z);

  // 2) создаём агента
  Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, y, z});
  m_agentManager.AddAgent(agent);

  // 3) объект сцены
  SceneObject obj;
  obj.agent_id = m_agentManager.getAgentCount() - 1;
  obj.object_type = prefab_id; // ключ в m_objectModels
  obj.position = glm::vec3(x, y, z);
  obj.scale = prefab.m_modelScale;
  obj.is_visible = true;
  m_sceneObjects.push_back(obj);
}

// Если нужно, оставь AddTree как шорткат:
void Scene::AddTree(float x, float z, const std::string &tree_type) {
  AddPrefab(tree_type, x, z);
}

// Список id префабов (проксируем)
const std::vector<std::string> &Scene::ListPrefabIds() const {
  return m_prefabSystem.ListIds();
}

void Scene::ReloadModelForPrefab(const std::string &prefab_id,
                                 const engine::data::AgentPrefab &prefab) {
  // Перезагрузка модели
  ObjectModels models;
  models.normal = std::make_shared<Model>(prefab.m_modelPath);
  models.burning = std::make_shared<Model>(prefab.m_modelPath);
  models.burned = std::make_shared<Model>(prefab.m_modelPath);
  m_objectModels[prefab_id] = std::move(models);

  // Обновляем префаб в системе (чтобы GetMaterialForState возвращал свежий
  // материал)
  auto &system_prefab = m_prefabSystem.GetPrefabMutable(prefab_id);
  system_prefab.normalMaterial = prefab.normalMaterial;
  system_prefab.burningMaterial = prefab.burningMaterial;
  system_prefab.burnedMaterial = prefab.burnedMaterial;

  // Обновляем scale для объектов сцены
  for (auto &obj : m_sceneObjects) {
    if (obj.object_type == prefab_id) {
      obj.scale = prefab.m_modelScale;
    }
  }

  std::cout << "[Scene] Reloaded model for prefab '" << prefab_id << "'\n";
}

// привязываем модель к префабу (если нужно или не нужно)
void Scene::AddSceneObjectFromSerialized(const SceneObject &obj) {
  const auto &prefab = m_prefabSystem.GetPrefab(obj.object_type);
  EnsureModelsForPrefab(obj.object_type, prefab);
  SceneObject o = obj;
  o.scale = prefab.m_modelScale;
  m_sceneObjects.push_back(o);
}

float Scene::GetGroundHeight(float x, float z) const {
  return m_terrain ? m_terrain->GetHeightAt(x, z) : 0.0F;
}

void Scene::AddFire(float x, float z, float intensity) {
  // теперь учитываем высоту земли и перенаправляем в 3D-вариант
  const float y = GetGroundHeight(x, z);
  AddFireAt(glm::vec3{x, y, z}, intensity);
}

void Scene::AddFireAt(const glm::vec3& world, float intensity) {
  // единая точка входа
  m_agentManager.AddFireAt(Float3{world.x, world.y, world.z}, intensity);
}

int Scene::FindClosestObjectXZ(const glm::vec3& world, float maxXZDist) const {
  int best = -1;
  float bestD2 = maxXZDist * maxXZDist;
  for (int i = 0; i < (int)m_sceneObjects.size(); ++i) {
    const auto& o = m_sceneObjects[i];
    if (!o.is_visible) continue;
    const float dx = o.position.x - world.x;
    const float dz = o.position.z - world.z;
    const float d2 = dx*dx + dz*dz;
    if (d2 <= bestD2) { bestD2 = d2; best = i; }
  }
  return best;
}

void Scene::RemoveSceneObjectByIndex(size_t idx) {
  if (idx >= m_sceneObjects.size()) return;
  // 1) удалить агента из симуляции с учётом swap-семантики
  const int removed_agent = m_sceneObjects[idx].agent_id;
  int swapped_from = -1;
  int swapped_to = -1;
  m_agentManager.RemoveAgentAt(removed_agent, &swapped_from, &swapped_to);

  // 2) если в менеджере произошёл swap, поправим agent_id у соответствующего SceneObject
  if (swapped_from >= 0 && swapped_to >= 0 && swapped_from != swapped_to) {
    for (auto& so : m_sceneObjects) {
      if (so.agent_id == swapped_from) {
        so.agent_id = swapped_to;
        break;
      }
    }
  }

  // 3) удалить объект сцены (swap pop_back для O(1))
  if (idx != m_sceneObjects.size() - 1) {
    std::swap(m_sceneObjects[idx], m_sceneObjects.back());
  }
  m_sceneObjects.pop_back();
}

bool Scene::RemoveNearestAt(const glm::vec3& world, float maxXZDist) {
  const int idx = FindClosestObjectXZ(world, maxXZDist);
  if (idx < 0) { return false;
}
  RemoveSceneObjectByIndex(static_cast<size_t>(idx));
  return true;
}







}  // namespace engine
