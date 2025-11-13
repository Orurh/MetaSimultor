#include "Scene.h"
#include "graphics/Model.h" 
#include <cuda_runtime.h>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <print>
#include <random>
#include <algorithm> 
#include <cmath> 
#include "core/Input.h"
#include "core/Window.h"
#include "core/agent.h"
#include "core/scene/Terrain.h"
#include "core/simulation.cuh"
#include "graphics/Renderer.h"


namespace engine {
Scene::Scene() : m_agentManager(1000000), m_camera(glm::vec3(500.0F, 100.0F, 600.0F)) {}

void Scene::Init() {
  m_terrain = std::make_unique<Terrain>();
  GetPrefabSystem().LoadAllFromDirectory("res/prefabs");
  engine::Renderer::Instance().Init();
}

void Scene::GenerateTerrain(const TerrainGenerationSettings& s) {
  m_terrain->GenerateRandom(s);
}

void Scene::EnsureModelsForPrefab(const std::string& id, const engine::data::AgentPrefab& prefab) {
  if (m_objectModels.Find(id) != nullptr) {
    return;
  }

  const std::string& path = prefab.m_modelPath;
  if (path.empty()) {
    std::cerr << "[Scene] prefab '" << id << "' has empty model_path\n";
    return;
  }

  LoadObjectModels(id, path, path, path);
}

void Scene::Update(float dt) {
  auto startTime = std::chrono::high_resolution_clock::now();

  m_windTime += dt;

  auto simStart = std::chrono::high_resolution_clock::now();
  if (!m_sceneObjects.Empty()) {
    SimulationStep(m_agentManager.getDeviceAgents(), m_agentManager.getAgentCount(), dt,
                   m_windConfig.getCurrentWind(m_windTime));
  }
  auto simTime = std::chrono::high_resolution_clock::now();

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
  if (!UI_MOUSE && engine::InputSystem::Instance().GetMouseButton(engine::MouseButton::Right,
                                                                  InputState::Held)) {
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
  const int FB_W = static_cast<int>(m_vpW *  m_vpScaleX);
  const int FB_H = static_cast<int>(m_vpH *  m_vpScaleY);
  glViewport(0, 0, FB_W, FB_H);

  const float ASPECT = FB_H > 0 ? static_cast<float>(FB_W) / static_cast<float>(FB_H) : 1.0F;
  auto projection = glm::perspective(m_camera.GetZoom(), ASPECT, 0.1F, 2000.0F);

  auto view = m_camera.GetViewMatrix();
  Renderer::Instance().BeginScene(view, projection);

  auto agents = LoadAgentsFromGPU();

  // Рендеринг объектов
  for (const auto& obj : m_sceneObjects.All()) {
    if (!obj.m_is_visible) {
      continue;
    }

    if (obj.m_agent_id < agents.size()) {
      const Agent& agent = agents[obj.m_agent_id];
      RenderAgent(agent, obj);
    }
  }

  Renderer::Instance().UploadAgentsData(agents);
  Renderer::Instance().DrawTerrain(m_terrain->GetVAO(),
                                   m_terrain->GetPosition());
  Renderer::Instance().RenderWithGeometryShader(
      m_windConfig.getCurrentWind(m_windTime));

  Renderer::Instance().EndScene();
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

  cudaMemcpy(agents.data(), m_agentManager.getDeviceAgents(), currentCount * sizeof(Agent),
             cudaMemcpyDeviceToHost);

  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::println("CUDA memcpy error: {}", cudaGetErrorString(err));
  }

  return agents;
}
void Scene::LoadObjectModels(const std::string& object_name, const std::string& normal_path,
                             const std::string& burning_path, const std::string& burned_path) {
  try {
    ObjectModels models;
    models.m_normal = std::make_shared<Model>(normal_path);
    models.m_burning = std::make_shared<Model>(burning_path);
    models.m_burned = std::make_shared<Model>(burned_path);
    m_objectModels.Put(object_name, std::move(models));
  } catch (const std::exception& e) {
    std::cout << "Failed to load model '" << object_name << "': " << e.what() << '\n';
  }
}

void Scene::RenderAgent(const Agent& agent, const SceneObject& scene_obj) {
  glm::vec3 position(agent.position.x, agent.position.y, agent.position.z);

  // Получаем префаб
  const auto& prefab = m_prefabSystem.GetPrefab(scene_obj.m_object_type);

  // Получаем модель
  const ObjectModels* om = m_objectModels.Find(scene_obj.m_object_type);
  if ((om == nullptr) || !om->m_normal) {
    std::cerr << "[RenderAgent] no model for '" << scene_obj.m_object_type << "'\n";
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
  auto model = om->m_normal;

  if (model && material) {
    Renderer::Instance().DrawModelWithMaterial(
        *model, position, scene_obj. m_scale, m_camera.GetPosition(), material);
  } else {
    // Fallback
    Renderer::Instance().DrawModel(*model, position, scene_obj. m_scale,
                                   m_camera.GetPosition());
  }
}

glm::mat4 Scene::GetProjMatrix(int w, int h) {
  return glm::perspective(m_camera.GetZoom(), static_cast<float>(w) / static_cast<float>(h), 0.1F,
                          2000.0F);
}

void Scene::Clear() {
  m_sceneObjects.Clear();
  m_agentManager.Clear();
}

// старая функция просто вызывает новую с 1.0
void Scene::AddPrefab(const std::string& id, float x, float z) {
  AddPrefabScaled(id, x, z, 1.0f, 0.0f);
}

// Если нужно, оставь AddTree как шорткат:
void Scene::AddTree(float x, float z, const std::string& tree_type) {
  AddPrefab(tree_type, x, z);
}

// Список id префабов (проксируем)
const std::vector<std::string>& Scene::ListPrefabIds() const {
  return m_prefabSystem.ListIds();
}

void Scene::ReloadModelForPrefab(const std::string& prefab_id,
                                 const engine::data::AgentPrefab& prefab) {
  // Перезагрузка модели
  ObjectModels models;
  models.m_normal = std::make_shared<Model>(prefab.m_modelPath);
  models.m_burning = std::make_shared<Model>(prefab.m_modelPath);
  models.m_burned = std::make_shared<Model>(prefab.m_modelPath);
  m_objectModels.Put(prefab_id, std::move(models));

  // Обновляем префаб в системе (чтобы GetMaterialForState возвращал свежий материал)
  auto& system_prefab = m_prefabSystem.GetPrefabMutable(prefab_id);
  system_prefab.normalMaterial = prefab.normalMaterial;
  system_prefab.burningMaterial = prefab.burningMaterial;
  system_prefab.burnedMaterial = prefab.burnedMaterial;

  // Обновляем  m_scale для объектов сцены
  for (auto& obj : m_sceneObjects.All()) {
    if (obj.m_object_type == prefab_id) {
      obj.m_scale = prefab.m_modelScale;
    }
  }
  std::cout << "[Scene] Reloaded model for prefab '" << prefab_id << "'\n";
}


// привязываем модель к префабу (если нужно или не нужно)
void Scene::AddSceneObjectFromSerialized(const SceneObject& obj) {
  const auto& prefab = m_prefabSystem.GetPrefab(obj.m_object_type);
  EnsureModelsForPrefab(obj.m_object_type, prefab);

  SceneObject o = obj;
  if (o.m_uniform_scale <= 0.f) o.m_uniform_scale = 1.0f; // на всякий случай
  o.m_scale = prefab.m_modelScale * o.m_uniform_scale;     // <<< важно
  m_sceneObjects.Add(std::move(o));
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
  return m_sceneObjects.FindClosestXZ(world, maxXZDist);
}

void Scene::RemoveSceneObjectByIndex(size_t idx) {
  const SceneObject* victim = m_sceneObjects.Get(idx);
  if (!victim) return;

  const int removed_agent = victim->m_agent_id;

  int swapped_from = -1;
  int swapped_to = -1;
  m_agentManager.RemoveAgentAt(removed_agent, &swapped_from, &swapped_to);

  m_sceneObjects.RemoveByIndex(idx);

  if (swapped_from >= 0 && swapped_to >= 0 && swapped_from != swapped_to) {
    for (auto& so : m_sceneObjects.All()) {
      if (so.m_agent_id == swapped_from) {
        so.m_agent_id = swapped_to;
        break;
      }
    }
  }
}


bool Scene::RemoveNearestAt(const glm::vec3& world, float maxXZDist) {
  const int idx = FindClosestObjectXZ(world, maxXZDist);
  if (idx < 0) {
    return false;
  }
  RemoveSceneObjectByIndex(static_cast<size_t>(idx));
  return true;
}

void Scene::AddPrefabScaled(const std::string& prefab_id,
                            float x, float z, float uniformScale, float /*yawDeg*/) {
  const auto& prefab = m_prefabSystem.GetPrefab(prefab_id);
  EnsureModelsForPrefab(prefab_id, prefab);
  const float y = GetGroundHeight(x, z);

  // 1) агент из префаба
  Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, y, z});

  // 2) адаптируем его коллайдер под визуальный масштаб
  const unsigned seed = std::hash<int>{}(static_cast<int>(x*31)) ^
                        (std::hash<int>{}(static_cast<int>(z*57)) << 1) ^
                        0x9e3779b9U;
  // ScaleAndAdaptCollider(agent.fire_collider, prefab.m_modelScale, uniformScale, seed);

  // 3) добавляем агента в симуляцию
  m_agentManager.AddAgent(agent);

  // 4) создаём SceneObject
  SceneObject obj;
  obj.m_agent_id       = m_agentManager.getAgentCount() - 1;
  obj.m_object_type    = prefab_id;
  obj.m_position       = glm::vec3(x, y, z);
  obj.m_uniform_scale  = uniformScale;                       // <<< НОВОЕ
  obj.m_scale          = prefab.m_modelScale;
  obj.m_is_visible     = true;
  obj.m_seedForCollider= seed;                               // (опц.) сохраняем

  m_sceneObjects.Add(std::move(obj));
}



}  // namespace engine
