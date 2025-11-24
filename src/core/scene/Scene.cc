#include "Scene.h"
#include "Entity.h"

#include "core/AgentManager.h"
#include "core/Input.h"
#include "core/Window.h"
#include "core/agent.h"
#include "core/scene/Terrain.h"
#include "graphics/Model.h"
#include "graphics/Renderer.h"

#include "graphics/ParticleSystem/ParticleSystemPool.h"
#include "utils/ScopeTimer.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
namespace engine {

Scene::Scene() : m_agentManager(1000000), m_camera(glm::vec3(500.0F, 100.0F, 600.0F)) { m_time.InitToNow(); }

void Scene::Init() {
  m_terrain = std::make_unique<Terrain>();
  GetPrefabSystem().LoadAllFromDirectory("res/prefabs");
  engine::Renderer::Instance().Init();
  ParticleSystemPool::Initialize(1000);

  m_heatMap.Init(256, 256);                                      // 256x256 resolution
  m_heatMap.SetWorldBounds(-100.0f, 1000.0f, -100.0f, 1000.0f);  // Настройте под ваш террейн
}

auto Scene::CreateEntity(const std::string &id) -> Entity {
  auto handle = m_registry.create();
  Entity entity(handle, &m_registry);

  entity.AddComponent<IDComponent>(id);
  return entity;
}

void Scene::GenerateTerrain(const TerrainGenerationSettings &s) {
  m_terrain->GenerateRandom(s);

  float padding = 50.0f;  // Небольшой отступZZ
  m_heatMap.SetWorldBounds(-padding, s.width * s.scale + padding, -padding, s.depth * s.scale + padding);
}

void Scene::EnsureModelsForPrefab(const std::string &id, const engine::data::AgentPrefab &prefab) {
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

  m_time.Update(static_cast<double>(dt));

  const float simDt = static_cast<float>(m_time.GetLastSimDt());
  const float simNow = static_cast<float>(m_time.GetSimTime());

  m_windTime += simNow;
  glm::vec3 currentWind = m_windConfig.getCurrentWind(m_windTime);
  if (m_agentManager.GetAgentCount() > 0 && simDt > 0.0f) {
    ScopeTimer timer("Simulation step profiling");
    m_fire.Step(m_agentManager, simDt, currentWind);
  }

  const bool UI_MOUSE = engine::InputSystem::Instance().UIMouseCaptured();
  const bool UI_KBD = engine::InputSystem::Instance().UIKeyboardCaptured();

  // if (!UI_KBD) {d
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
  // }

  // Поворот камеры
  const glm::vec2 DELTA = engine::InputSystem::Instance().GetMouseDelta();
  if (!UI_MOUSE && engine::InputSystem::Instance().GetMouseButton(engine::MouseButton::Right, InputState::Held)) {
    if (DELTA.x != 0.F || DELTA.y != 0.F) {
      m_camera.ProcessMouseMovement(DELTA.x, DELTA.y);
    }
  }

  if (!UI_MOUSE) {
    const float SCROLL_Y = engine::InputSystem::Instance().GetMouseScrollY();
    if (SCROLL_Y != 0.0F) {
    }
  }
  auto agents = LoadAgentsFromGPU();
  float ambientTemp = m_fire.Config().m_env.m_ambient_temperature;
  m_heatMap.UpdateFromAgents(agents, ambientTemp);

  auto particleAgentView = m_registry.view<ParticleSystemComponent, AgentLinkComponent>();
  for (auto [entity, particleComp, link] : particleAgentView.each()) {
    if (link.gpu_index < agents.size()) {
      const Agent &agent = agents[link.gpu_index];

      if ((agent.isBurning() || agent.type == FireAgentState::Burning) && !particleComp.isActive) {
        particleComp.isActive = true;
        particleComp.system->Play();
        particleComp.activationTimer = 2.0f;
      } else if (!agent.isBurning() && agent.type != FireAgentState::Burning && particleComp.isActive) {
        particleComp.isActive = false;
        particleComp.system->Stop();
      }
    }
  }

  auto particleView = m_registry.view<ParticleSystemComponent>();
  for (auto [entity, particleComp] : particleView.each()) {
    if (particleComp.isActive) {
      particleComp.system->SetWind(currentWind);
      particleComp.system->Update();

      if (!particleComp.system->IsPlaying() && particleComp.activationTimer > 0.0f && simDt > 0.0f) {
        particleComp.activationTimer -= simDt;
        if (particleComp.activationTimer <= 0.0f) {
          particleComp.isActive = false;
        }
      }
    }
  }

  for (auto &system : m_particleSystems) {
    system->SetWind(currentWind);
    system->Update();
  }
}

void Scene::Render() {
  ScopeTimer timer("Render Time:");

  const int FB_W = static_cast<int>(m_vpW * m_vpScaleX);
  const int FB_H = static_cast<int>(m_vpH * m_vpScaleY);
  glViewport(m_vpX, m_vpY, FB_W, FB_H);

  const float aspect = FB_H > 0 ? FB_W / (float)FB_H : 1.0f;
  auto projection = glm::perspective(m_camera.GetZoom(), aspect, 0.1f, 2000.0f);
  auto view = m_camera.GetViewMatrix();

  Renderer::Instance().BeginScene(view, projection);
  auto agents = LoadAgentsFromGPU();

  std::unordered_map<std::string, std::vector<glm::mat4>> instanceData;

  auto render_view = m_registry.view<TransformComponent, RenderComponent, AgentLinkComponent>();
  for (auto [entity, transform, render, link] : render_view.each()) {
    if (!render.is_visible)
      continue;

    const Agent &agent = agents[link.gpu_index];

    FireAgentState state = FireAgentState::Normal;
    if (agent.type == FireAgentState::Burned)
      state = FireAgentState::Burned;
    else if (agent.isBurning() || agent.type == FireAgentState::Burning)
      state = FireAgentState::Burning;

    glm::mat4 modelMatrix = CalculateModelMatrix(transform.position, render.scale);
    std::string instanceKey = link.prefab_id + "_" + std::to_string(static_cast<int>(state));

    instanceData[instanceKey].push_back(modelMatrix);
  }

  RenderInstancedModels(instanceData);

  Renderer::Instance().UploadAgentsData(agents);
  auto particleView = m_registry.view<ParticleSystemComponent, TransformComponent>();
  {
    ScopeTimer timer("ParticleSystem::Render All Systems");
    for (auto [entity, particleComp, transform] : particleView.each()) {
      if (particleComp.isActive) {
        particleComp.system->SetPosition(transform.position);
        particleComp.system->Render(view, projection);
      }
    }
  }

  for (auto &system : m_particleSystems) {
    system->Render(view, projection);
  }
  Renderer::Instance().DrawTerrain(m_terrain->GetVAO(), m_terrain->GetPosition(), m_terrain->GetDiffuseTexture());

  Renderer::Instance().EndScene();
}

void Scene::AddParticleSystem(const std::shared_ptr<ParticleSystem> &system) { m_particleSystems.push_back(system); }

void Scene::RemoveParticleSystem(const std::shared_ptr<ParticleSystem> &system) {
  m_particleSystems.erase(std::remove(m_particleSystems.begin(), m_particleSystems.end(), system),
                          m_particleSystems.end());
}

void Scene::Generate() {
  for (int i = 0; i < 50; i += 3) {
    for (int j = 0; j < 50; j += 3) {
      AddTree(500 + i, 500 + j, "default_tree");
    }
  }
}

std::vector<Agent> Scene::LoadAgentsFromGPU() { return m_agentManager.LoadAgentsFromGPU(); }
void Scene::LoadObjectModels(const std::string &object_name, const std::string &normal_path,
                             const std::string &burning_path, const std::string &burned_path) {
  try {
    ObjectModels models;
    models.m_normal = std::make_shared<Model>(normal_path);
    models.m_burning = std::make_shared<Model>(burning_path);
    models.m_burned = std::make_shared<Model>(burned_path);
    m_objectModels.Put(object_name, std::move(models));
  } catch (const std::exception &e) {
    std::cout << "Failed to load model '" << object_name << "': " << e.what() << '\n';
  }
}

glm::mat4 Scene::GetProjMatrix(int w, int h) {
  return glm::perspective(m_camera.GetZoom(), static_cast<float>(w) / static_cast<float>(h), 0.1F, 2000.0F);
}

void Scene::Clear() {
  m_agentManager.Clear();
  m_registry.clear();
}

void Scene::Shutdown() {
  m_objectModels.Clear();

  m_prefabSystem.ReleaseAllMaterials();

  m_agentManager.Clear();
  m_terrain->Cleanup();
  Renderer::Instance().Shutdown();
}

void Scene::AddPrefab(const std::string &prefab_id, float x, float z, float scale) {
  const auto &prefab = m_prefabSystem.GetPrefab(prefab_id);
  EnsureModelsForPrefab(prefab_id, prefab);
  const float y = GetGroundHeight(x, z);

  glm::vec3 finalScale = prefab.m_modelScale * scale;

  // 1. GPU агент
  Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, y, z}, scale);

  // 🔥 Применяем материал огня из префаба
  // prefab.m_fireMaterialId / m_fireIgnitionMult / m_fireMoistureOffset
  m_fire.ApplyMaterialToAgent(agent, prefab.m_fireMaterialId);

  // Небольшие модификаторы из префаба
  auto &coll = agent.fire_collider;

  // ignition_temp у тебя скаляр — скейлим один раз
  coll.ignition_temp *= prefab.m_fireIgnitionMult;

  // moisture — массив по точкам
  for (int i = 0; i < coll.point_count; ++i) {
    coll.moisture[i] += prefab.m_fireMoistureOffset;
    coll.fuels[i] *= prefab.m_fireFuelMult;  // добавил множитель топлива
  }

  int gpu_index = m_agentManager.AddAgent(agent);

  // 2. ECS Entity
  Entity entity = CreateEntity(prefab_id);
  entity.AddComponent<TransformComponent>(glm::vec3{x, y, z});
  entity.AddComponent<RenderComponent>(prefab_id, finalScale, true);
  entity.AddComponent<AgentLinkComponent>(gpu_index, prefab_id);

  auto &particleComp = entity.AddComponent<ParticleSystemComponent>();
  particleComp.system = ParticleSystemPool::AcquireFire();

  // ставим систему частиц примерно над объектом
  glm::vec3 particlePos{x, y + finalScale.y, z};
  particleComp.system->SetPosition(particlePos);

  particleComp.system->Stop();
  particleComp.isActive = false;

  // 3. FireEmitter
  if (prefab_id.find("fire") != std::string::npos) {
    entity.AddComponent<FireEmitterComponent>();
  }
}

void Scene::AddTree(float x, float z, const std::string &tree_type, float scale) { AddPrefab(tree_type, x, z, scale); }

const std::vector<std::string> &Scene::ListPrefabIds() const { return m_prefabSystem.ListIds(); }

void Scene::ReloadModelForPrefab(const std::string &prefab_id, const engine::data::AgentPrefab &prefab) {
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

  auto view = m_registry.view<RenderComponent>();
  for (auto [entity, render] : view.each()) {
    if (render.prefab_id == prefab_id) {
      render.scale = prefab.m_modelScale;
    }
  }

  std::cout << "[Scene] Reloaded model for prefab '" << prefab_id << "'\n";
}

float Scene::GetGroundHeight(float x, float z) const { return m_terrain ? m_terrain->GetHeightAt(x, z) : 0.0F; }

void Scene::AddFire(float x, float z, float intensity) {
  const float y = GetGroundHeight(x, z);
  AddFireAt(glm::vec3{x, y, z}, intensity);
}

void Scene::AddFireAt(const glm::vec3 &world, float intensity) {
  std::cout << "[Fire] AddFireAt world = (" << world.x << ", " << world.y << ", " << world.z
            << "), intensity = " << intensity << '\n';
  m_agentManager.AddFireAt(Float3{world.x, world.y, world.z}, intensity);

  Entity fireEntity = CreateEntity("fire_emitter");
  fireEntity.AddComponent<TransformComponent>(world);

  auto &particleComp = fireEntity.AddComponent<ParticleSystemComponent>();
  particleComp.system = ParticleSystemPool::AcquireFire();
  particleComp.system->SetPosition(world);
  particleComp.system->Play();
  particleComp.isActive = true;

  fireEntity.AddComponent<FireEmitterComponent>();
}
bool Scene::RemoveNearestAt(const glm::vec3 &world, float maxXZDist) {
  float bestDist2 = maxXZDist * maxXZDist;
  entt::entity best = entt::null;

  auto view = m_registry.view<TransformComponent, AgentLinkComponent>();
  view.each([&](entt::entity entity, const TransformComponent &transform, const AgentLinkComponent &link) {
    float dx = transform.position.x - world.x;
    float dz = transform.position.z - world.z;
    float dist2 = dx * dx + dz * dz;
    if (dist2 < bestDist2) {
      bestDist2 = dist2;
      best = entity;
    }
  });

  if (m_registry.all_of<ParticleSystemComponent>(best)) {
    m_registry.remove<ParticleSystemComponent>(best);
  }

  if (best == entt::null)
    return false;

  auto &link = m_registry.get<AgentLinkComponent>(best);
  int gpu_idx = link.gpu_index;

  int swapped_from = -1, swapped_to = -1;
  if (!m_agentManager.RemoveAgentAt(gpu_idx, &swapped_from, &swapped_to)) {
    return false;
  }

  if (swapped_from != -1 && swapped_to != -1 && swapped_from != swapped_to) {
    auto update_view = m_registry.view<AgentLinkComponent>();
    for (auto [entity, link_comp] : update_view.each()) {
      if (link_comp.gpu_index == swapped_from) {
        link_comp.gpu_index = swapped_to;
        break;
      }
    }
  }

  m_registry.destroy(best);

  // Удаляем из мапы по ID
  if (m_registry.all_of<IDComponent>(best)) {
    auto &id = m_registry.get<IDComponent>(best);
  }

  return true;
}

// воспламеняет агента
void Scene::IgniteNearest(const glm::vec3 &worldPos, float maxDist) {
  // auto agents = LoadAgentsFromGPU();

  // for (Agent& ag : agents) {
  //   auto& coll = ag.fire_collider;

  //   // воспламеняем все точки
  //   for (int i = 0; i < coll.point_count; i++) {
  //     coll.temperatures[i] = coll.ignition_temp + 50.0f;  // чуть выше порога
  //     coll.intensities[i] = 1.0f;                         // максимум
  //     coll.fuels[i] = std::max(coll.fuels[i], 50.0f);
  //   }

  //   ag.type = FireAgentState::Burning;
  // }

  // m_agentManager.SetAgents(agents);

  auto agents = LoadAgentsFromGPU();

  int best = -1;
  float bestDist2 = maxDist * maxDist;

  // найдём ближайшего агента
  auto view = m_registry.view<TransformComponent, AgentLinkComponent>();
  for (auto [entity, transform, link] : view.each()) {
    float dx = transform.position.x - worldPos.x;
    float dz = transform.position.z - worldPos.z;
    float d2 = dx * dx + dz * dz;
    if (d2 < bestDist2) {
      bestDist2 = d2;
      best = link.gpu_index;
    }
  }

  if (best == -1)
    return;

  Agent &ag = agents[best];
  auto &coll = ag.fire_collider;

  // воспламеняем все точки
  for (int i = 0; i < coll.point_count; i++) {
    coll.temperatures[i] = coll.ignition_temp + 50.0f;  // чуть выше порога
    coll.intensities[i] = 1.0f;                         // максимум
    coll.fuels[i] = std::max(coll.fuels[i], 50.0f);
  }

  ag.type = FireAgentState::Burning;
  m_agentManager.SetAgents(agents);
}
void Scene::RenderInstancedModels(const std::unordered_map<std::string, std::vector<glm::mat4>> &instanceData) {
  for (const auto &[key, matrices] : instanceData) {
    // Разбираем ключ на prefab_id и состояние
    size_t underscorePos = key.find_last_of('_');
    std::string prefabId = key.substr(0, underscorePos);
    FireAgentState state = static_cast<FireAgentState>(std::stoi(key.substr(underscorePos + 1)));

    auto model = m_objectModels.Find(prefabId);
    auto material = m_prefabSystem.GetPrefab(prefabId).GetMaterialForState(state);

    if (model && material) {
      auto *draw_model = GetModelForState(model, state);
      if (draw_model) {
        Renderer::Instance().DrawModelInstanced(*draw_model, matrices, m_camera.GetPosition(), material);
      }
    }
  }
}

Model *Scene::GetModelForState(const ObjectModels *models, FireAgentState state) {
  if (!models)
    return nullptr;

  switch (state) {
  case FireAgentState::Burning:
    if (models->m_burning)
      return models->m_burning.get();
  case FireAgentState::Burned:
    if (models->m_burned)
      return models->m_burned.get();
  case FireAgentState::Normal:
  default:
    return models->m_normal ? models->m_normal.get() : nullptr;
  }
}

glm::mat4 Scene::CalculateModelMatrix(const glm::vec3 &position, const glm::vec3 &scale) const {
  glm::mat4 modelMatrix = glm::mat4(1.0f);
  modelMatrix = glm::translate(modelMatrix, position);
  modelMatrix = glm::scale(modelMatrix, scale);
  return modelMatrix;
}

}  // namespace engine
