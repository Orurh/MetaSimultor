#include "Scene.h"
#include "Entity.h"
#include "core/Input.h"
#include "core/Window.h"
#include "core/agent.h"
#include "core/scene/Terrain.h"
#include "core/simulation.cuh"
#include "graphics/Model.h"
#include "graphics/Renderer.h"
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
  GetPrefabSystem().LoadAllFromDirectory("res/prefabs");
  engine::Renderer::Instance().Init();
}

auto Scene::CreateEntity(const std::string &id) -> Entity {
  Entity entity = {m_Registry.create(), this};
  entity.AddComponent<IDComponent>(id);
  m_EntityMap[id] = entity;
  return entity;
}

void Scene::GenerateTerrain(const TerrainGenerationSettings &s) {
  m_terrain->GenerateRandom(s);
}

void Scene::EnsureModelsForPrefab(const std::string &id,
                                  const engine::data::AgentPrefab &prefab) {
  if (m_objectModels.Find(id)) {
    return;
  }

  const std::string &path = prefab.m_modelPath;
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
  if (m_agentManager.getAgentCount() > 0) {
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
  glViewport(m_vpX, m_vpY, FB_W, FB_H);

  const float aspect = FB_H > 0 ? FB_W / (float)FB_H : 1.0f;
  auto projection = glm::perspective(m_camera.GetZoom(), aspect, 0.1f, 2000.0f);
  auto view = m_camera.GetViewMatrix();

  Renderer::Instance().BeginScene(view, projection);

  // 1. Синхронизация GPU → CPU
  auto agents = LoadAgentsFromGPU(); // ← m_agentManager

  // 2. Рендер через ECS
  auto render_view =
      m_Registry
          .view<TransformComponent, RenderComponent, AgentLinkComponent>();
  for (auto [entity, transform, render, link] : render_view.each()) {
    if (!render.is_visible)
      continue;

    const Agent &agent = agents[link.gpu_index];
    const auto &prefab = m_prefabSystem.GetPrefab(link.prefab_id);

    // Определяем состояние
    AgentType state = AgentType::NORMAL;
    if (agent.type == AgentType::BURNED)
      state = AgentType::BURNED;
    else if (agent.isBurning() || agent.type == AgentType::BURNING)
      state = AgentType::BURNING;

    auto material = prefab.GetMaterialForState(state);
    auto model = m_objectModels.Find(link.prefab_id);
    if (!model || !model->m_normal)
      continue;

    auto *draw_model = model->m_normal.get();
    if (state == AgentType::BURNING && model->m_burning)
      draw_model = model->m_burning.get();
    if (state == AgentType::BURNED && model->m_burned)
      draw_model = model->m_burned.get();

    if (draw_model && material) {
      Renderer::Instance().DrawModelWithMaterial(
          *draw_model, transform.position, render.scale, m_camera.GetPosition(),
          material);
    }
  }

  // 3. Частицы + Terrain
  Renderer::Instance().UploadAgentsData(agents);
  Renderer::Instance().RenderWithGeometryShader(
      m_windConfig.getCurrentWind(m_windTime));
  Renderer::Instance().DrawTerrain(m_terrain->GetVAO(),
                                   m_terrain->GetPosition());

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
    models.m_normal = std::make_shared<Model>(normal_path);
    models.m_burning = std::make_shared<Model>(burning_path);
    models.m_burned = std::make_shared<Model>(burned_path);
    m_objectModels.Put(object_name, std::move(models));
  } catch (const std::exception &e) {
    std::cout << "Failed to load model '" << object_name << "': " << e.what()
              << '\n';
  }
}

void Scene::RenderAgent(const Agent &agent, const SceneObject &scene_obj) {
  glm::vec3 position(agent.position.x, agent.position.y, agent.position.z);

  // Получаем префаб
  const auto &prefab = m_prefabSystem.GetPrefab(scene_obj.m_object_type);

  // Получаем модель
  const ObjectModels *om = m_objectModels.Find(scene_obj.m_object_type);
  if ((om == nullptr) || !om->m_normal) {
    std::cerr << "[RenderAgent] no model for '" << scene_obj.m_object_type
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
  auto model = om->m_normal;

  if (model && material) {
    Renderer::Instance().DrawModelWithMaterial(
        *model, position, scene_obj.m_scale, m_camera.GetPosition(), material);
  } else {
    // Fallback
    Renderer::Instance().DrawModel(*model, position, scene_obj.m_scale,
                                   m_camera.GetPosition());
  }
}

glm::mat4 Scene::GetProjMatrix(int w, int h) {
  return glm::perspective(m_camera.GetZoom(),
                          static_cast<float>(w) / static_cast<float>(h), 0.1F,
                          2000.0F);
}

void Scene::Clear() {
  m_agentManager.Clear();
  m_Registry.clear();
  m_EntityMap.clear();
}

void Scene::Shutdown() {
  m_objectModels.Clear();

  m_prefabSystem.ReleaseAllMaterials();

  m_agentManager.Clear();
  m_terrain->Cleanup();
  Renderer::Instance().Shutdown();
}

void Scene::AddPrefab(const std::string &prefab_id, float x, float z) {
  const auto &prefab = m_prefabSystem.GetPrefab(prefab_id);
  EnsureModelsForPrefab(prefab_id, prefab);
  const float y = GetGroundHeight(x, z);

  // 1. GPU агент
  Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, y, z});
  int gpu_index = m_agentManager.AddAgent(agent);

  // 2. ECS Entity
  Entity entity = CreateEntity(prefab_id);
  entity.AddComponent<TransformComponent>(glm::vec3{x, y, z});
  entity.AddComponent<RenderComponent>(prefab_id, prefab.m_modelScale, true);
  entity.AddComponent<AgentLinkComponent>(gpu_index, prefab_id);

  // 3. FireEmitter
  if (prefab_id.find("fire") != std::string::npos) {
    entity.AddComponent<FireEmitterComponent>();
  }
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
  models.m_normal = std::make_shared<Model>(prefab.m_modelPath);
  models.m_burning = std::make_shared<Model>(prefab.m_modelPath);
  models.m_burned = std::make_shared<Model>(prefab.m_modelPath);
  m_objectModels.Put(prefab_id, std::move(models));

  // 2. Обновляем префаб (материалы)
  auto &system_prefab = m_prefabSystem.GetPrefabMutable(prefab_id);
  system_prefab.normalMaterial = prefab.normalMaterial;
  system_prefab.burningMaterial = prefab.burningMaterial;
  system_prefab.burnedMaterial = prefab.burnedMaterial;

  // 3. Обновляем scale у всех Entity с этим prefab_id
  auto view = m_Registry.view<RenderComponent>();
  for (auto [entity, render] : view.each()) {
    if (render.prefab_id == prefab_id) {
      render.scale = prefab.m_modelScale;
    }
  }

  std::cout << "[Scene] Reloaded model for prefab '" << prefab_id << "'\n";
}

float Scene::GetGroundHeight(float x, float z) const {
  return m_terrain ? m_terrain->GetHeightAt(x, z) : 0.0F;
}

void Scene::AddFire(float x, float z, float intensity) {
  // теперь учитываем высоту земли и перенаправляем в 3D-вариант
  const float y = GetGroundHeight(x, z);
  AddFireAt(glm::vec3{x, y, z}, intensity);
}

void Scene::AddFireAt(const glm::vec3 &world, float intensity) {
  m_agentManager.AddFireAt(Float3{world.x, world.y, world.z}, intensity);
}
bool Scene::RemoveNearestAt(const glm::vec3 &world, float maxXZDist) {
  float bestDist2 = maxXZDist * maxXZDist;
  entt::entity best = entt::null;

  auto view = m_Registry.view<TransformComponent, AgentLinkComponent>();
  view.each([&](entt::entity entity, const TransformComponent &transform,
                const AgentLinkComponent &link) {
    float dx = transform.position.x - world.x;
    float dz = transform.position.z - world.z;
    float dist2 = dx * dx + dz * dz;
    if (dist2 < bestDist2) {
      bestDist2 = dist2;
      best = entity;
    }
  });

  if (best == entt::null)
    return false;

  auto &link = m_Registry.get<AgentLinkComponent>(best);
  int gpu_idx = link.gpu_index;

  int swapped_from = -1, swapped_to = -1;
  if (!m_agentManager.RemoveAgentAt(gpu_idx, &swapped_from, &swapped_to)) {
    return false;
  }

  if (swapped_from != -1 && swapped_to != -1 && swapped_from != swapped_to) {
    auto update_view = m_Registry.view<AgentLinkComponent>();
    for (auto [entity, link_comp] : update_view.each()) {
      if (link_comp.gpu_index == swapped_from) {
        link_comp.gpu_index = swapped_to;
        break;
      }
    }
  }

  m_Registry.destroy(best);

  // Удаляем из мапы по ID
  if (m_Registry.all_of<IDComponent>(best)) {
    auto &id = m_Registry.get<IDComponent>(best);
    m_EntityMap.erase(id.ID);
  }

  return true;
}
} // namespace engine
