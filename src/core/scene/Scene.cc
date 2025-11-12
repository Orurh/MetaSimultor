#include "Scene.h"
#include "graphics/Model.h" 
#include <cuda_runtime.h>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <print>
#include <random>
#include "core/Input.h"
#include "core/Window.h"
#include "core/agent.h"
#include "core/scene/Terrain.h"
#include "core/simulation.cuh"
#include "graphics/Renderer.h"

namespace {
inline Float3 mul(const Float3& a, const glm::vec3& s) {
  return Float3(a.x * s.x, a.y * s.y, a.z * s.z);
}

// равномерная «средняя» шкала (можно заменить на max(s.x,s.y,s.z))
inline float uniform_of(const glm::vec3& s) {
  return (s.x + s.y + s.z) / 3.0f;
}

void ScaleCollider(FireCollider& col, const glm::vec3& finalScale) {
  col.bounds_min = mul(col.bounds_min, finalScale);
  col.bounds_max = mul(col.bounds_max, finalScale);
  for (int i = 0; i < col.point_count; ++i) {
    col.heat_points[i] = mul(col.heat_points[i], finalScale);
  }
}

// простая ресэмплинг-сеточка внутри AABB, чтобы точек стало больше на крупных объектах
void ResampleHeatPoints(FireCollider& col, int desiredCount, unsigned seed = 0xC0FFEEu) {
  desiredCount = std::max(1, std::min(desiredCount, 64));
  const Float3 mn = col.bounds_min;
  const Float3 mx = col.bounds_max;
  const float sx = std::max(1e-5f, mx.x - mn.x);
  const float sy = std::max(1e-5f, mx.y - mn.y);
  const float sz = std::max(1e-5f, mx.z - mn.z);

  // решётка ~ кубический корень
  const int n = static_cast<int>(std::ceil(std::cbrt(static_cast<float>(desiredCount))));
  const int total = std::min(64, n * n * n);

  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> j(-0.15f, 0.15f); // чуть «дышим»

  int k = 0;
  for (int iz = 0; iz < n && k < total; ++iz) {
    for (int iy = 0; iy < n && k < total; ++iy) {
      for (int ix = 0; ix < n && k < total; ++ix) {
        const float fx = (ix + 0.5f) / n + j(rng);
        const float fy = (iy + 0.5f) / n + j(rng);
        const float fz = (iz + 0.5f) / n + j(rng);
        const float px = mn.x + std::clamp(fx, 0.f, 1.f) * sx;
        const float py = mn.y + std::clamp(fy, 0.f, 1.f) * sy;
        const float pz = mn.z + std::clamp(fz, 0.f, 1.f) * sz;

        col.heat_points[k] = Float3(px, py, pz);
        // базовые значения: пусть топлива больше у больших
        col.fuels[k]       = 1.0f;
        col.intensities[k] = 0.0f;
        ++k;
      }
    }
  }
  col.point_count = k;
}

// масштабирующий апдейт коллайдера + плотности точек
void ScaleAndAdaptCollider(FireCollider& col,
                           const glm::vec3& prefabModelScale, // базовая шкала из префаба
                           float uniformScale,                 // множитель из инструмента
                           unsigned seedForSampling) {
  const glm::vec3 finalScale = prefabModelScale * uniformScale;

  // 1) масштаб геометрии коллайдера
  ScaleCollider(col, finalScale);

  // 2) подберём целевое кол-во точек ~ площади XZ (часто логичнее, чем по объёму)
  //    N_target = clamp(round(N_base * (uniformScale^alpha)), 1..64)
  //    где alpha ~ 1.5 даёт компромисс между площадью(2) и линейной(1)
  const float alpha = 1.5f;
  const int   baseN = std::max(1, col.point_count); // то, что пришло из префаба
  const int   targetN = std::max(1, std::min(64,
                           static_cast<int>(std::lround(baseN * std::pow(uniformScale, alpha)))) );

  // 3) ресэмплинг (равномерно по AABB; если нужно, можно учесть «крону» и т.п.)
  ResampleHeatPoints(col, targetN, seedForSampling);
}
} // anonymous




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
  if (m_objectModels.Find(id)) {
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
  // (1) Перезагрузка моделей в хранилище
  ObjectModels models;
  models.m_normal  = std::make_shared<Model>(prefab.m_modelPath);
  models.m_burning = std::make_shared<Model>(prefab.m_modelPath);
  models.m_burned  = std::make_shared<Model>(prefab.m_modelPath);
  m_objectModels.Put(prefab_id, std::move(models));

  // (2) Обновляем материалы/параметры префаба в системе префабов
  auto& system_prefab = m_prefabSystem.GetPrefabMutable(prefab_id);
  system_prefab.normalMaterial  = prefab.normalMaterial;
  system_prefab.burningMaterial = prefab.burningMaterial;
  system_prefab.burnedMaterial  = prefab.burnedMaterial;
  system_prefab.m_modelScale    = prefab.m_modelScale;  // не забыть обновить и базовый scale

  // (3) Обновляем объекты сцены и их коллайдеры у соответствующих агентов
  for (auto& obj : m_sceneObjects.All()) {
    if (obj.m_object_type != prefab_id) { continue;
}

    // 3.1 визуальный масштаб
    obj.m_scale = prefab.m_modelScale * obj.m_uniform_scale;

    // 3.2 обновить коллайдер агента под новый базовый scale
    if (obj.m_agent_id >= 0 && obj.m_agent_id < m_agentManager.getAgentCount()) {
      // Вариант A: через методы доступа к CPU-буферу одного агента:
      Agent ag = m_agentManager.GetAgentCPU(obj.m_agent_id); // ← добавь метод
      const unsigned seed = (obj.m_seedForCollider != 0)
                            ? obj.m_seedForCollider
                            : (std::hash<int>{}(obj.m_position.x*31) ^
                               (std::hash<int>{}(obj.m_position.z*57) << 1) ^ 0x9e3779b9U);
      ScaleAndAdaptCollider(ag.fire_collider, prefab.m_modelScale, obj.m_uniform_scale, seed);
      m_agentManager.UpdateAgentCPU(obj.m_agent_id, ag);     // ← добавь метод
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
  ScaleAndAdaptCollider(agent.fire_collider, prefab.m_modelScale, uniformScale, seed);

  // 3) добавляем агента в симуляцию
  m_agentManager.AddAgent(agent);

  // 4) создаём SceneObject
  SceneObject obj;
  obj.m_agent_id       = m_agentManager.getAgentCount() - 1;
  obj.m_object_type    = prefab_id;
  obj.m_position       = glm::vec3(x, y, z);
  obj.m_uniform_scale  = uniformScale;                       // <<< НОВОЕ
  obj.m_scale          = prefab.m_modelScale * uniformScale; // визуальный итог
  obj.m_is_visible     = true;
  obj.m_seedForCollider= seed;                               // (опц.) сохраняем

  m_sceneObjects.Add(std::move(obj));
}



}  // namespace engine
