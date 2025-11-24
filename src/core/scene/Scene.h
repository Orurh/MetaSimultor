#pragma once
#include <memory>
#include "Camera.h"
#include "Terrain.h"
#include "configs/WindConfig.h"
#include "core/AgentManager.h"
#include "core/fire/FireConfig.h"
#include "data/PrefabSystem.h"
#include "graphics/ParticleSystem/ParticleSystem.h"

#include "Components.h"
#include "core/fire/FireModule.h"
#include "entt.hpp"
#include "sceneObj/ObjectModelStore.h"
#include "core/scene/heatMap.h"

#include "SimulationTime.h"

namespace engine {
class Model;
class Entity;
class Material;
class Scene {

 public:
  Scene();
  void Init();
  void Update(float dt);
  auto CreateEntity(const std::string& id) -> Entity;
  void Render();
  void AddTree(float x, float z, const std::string& tree_type = "default_tree", float scale = 1.F);
  void AddFire(float x, float z, float intensity = 1.0F);

  void AddFireAt(const glm::vec3& world, float intensity = 1.0F);
  void IgniteNearest(const glm::vec3& worldPos, float maxDist);
  void Generate();
  void Clear();
  void Shutdown();
  void SetFireIntensity(float v) { m_fireIntensity = v; }
  std::vector<Agent> LoadAgentsFromGPU();
  auto GetSceneObjectsView() const { return m_registry.view<TransformComponent, RenderComponent, AgentLinkComponent>(); }

  glm::mat4 GetViewMatrix() { return m_camera.GetViewMatrix(); }
  glm::mat4 GetProjMatrix(int w, int h);
  AgentManager& GetAgentManager() { return m_agentManager; }

  void SetViewportRect(int x, int y, int w, int h) {
    m_vpX = x;
    m_vpY = y;
    m_vpW = w;
    m_vpH = h;
  }
  void SetViewportScale(float sx, float sy) {
    m_vpScaleX = sx;
    m_vpScaleY = sy;
  }

  void AddPrefab(const std::string& prefab_id, float x, float z, float scale);
  bool RemoveNearestAt(const glm::vec3& world, float maxXZDist);

  const std::vector<std::string>& ListPrefabIds() const;

  engine::data::PrefabSystem& GetPrefabSystem() { return m_prefabSystem; }
  const engine::data::PrefabSystem& GetPrefabSystem() const { return m_prefabSystem; }

  WindConfig& GetWindConfig() { return m_windConfig; }
  const WindConfig& GetWindConfig() const { return m_windConfig; }

  float GetWindTime() const { return m_windTime; }
  void ReloadModelForPrefab(const std::string& prefab_id, const engine::data::AgentPrefab& prefab);
  void GenerateTerrain(const TerrainGenerationSettings& s);

  Terrain* GetTerrain() { return m_terrain.get(); }
  const Terrain* GetTerrain() const { return m_terrain.get(); }
  float GetGroundHeight(float x, float z) const;
  size_t GetAgentCount() const { return m_agentManager.GetAgentCount(); }
  void AddParticleSystem(const std::shared_ptr<ParticleSystem>& system);
  void RemoveParticleSystem(const std::shared_ptr<ParticleSystem>& system);
  const std::vector<std::shared_ptr<ParticleSystem>>& GetParticleSystems() const { return m_particleSystems; }

  FireModule& GetFireModule() { return m_fire; }
  const FireModule& GetFireModule() const { return m_fire; }

  SimulationTime& GetTime() { return m_time; }
  const SimulationTime& GetTime() const { return m_time; }

  double GetSimTimeSeconds() const { return m_time.GetSimTime(); }
  double GetRealTimeSeconds() const { return m_time.GetRealTimeSinceStart(); }


  void ToggleHeatMap() { m_showHeatMap = !m_showHeatMap; }
  bool IsHeatMapVisible() const { return m_showHeatMap; }
  void SetHeatMapVisible(bool visible) { m_showHeatMap = visible; }
  HeatMap& GetHeatMap() { return m_heatMap; }

 private:
  void EnsureModelsForPrefab(const std::string& id, const engine::data::AgentPrefab& prefab);
  void LoadObjectModels(const std::string& object_name, const std::string& normal_path, const std::string& burning_path,
                        const std::string& burned_path);
  void RenderInstancedModels(const std::unordered_map<std::string, std::vector<glm::mat4>>& instanceData);

  Model* GetModelForState(const ObjectModels* models, FireAgentState state);
  glm::mat4 CalculateModelMatrix(const glm::vec3& position, const glm::vec3& scale) const;
  AgentManager m_agentManager;
  std::vector<std::shared_ptr<ParticleSystem>> m_particleSystems;

  entt::registry m_registry;
  std::unique_ptr<Terrain> m_terrain;
  data::PrefabSystem m_prefabSystem;
  ObjectModelStore m_objectModels;
  Camera m_camera;
  WindConfig m_windConfig;
  FireConfig m_fireConfig;
  float m_windTime = 0.0F;
  float m_fireIntensity{1.0F};
  std::shared_ptr<ParticleSystem> m_firePrototype;

  void InitializeParticlePrototypes() {
    m_firePrototype = std::make_shared<ParticleSystem>();
    m_firePrototype->SetFireEffect();
    m_firePrototype->Stop();
  }
  int m_vpX{0}, m_vpY{0};
  int m_vpW{1920}, m_vpH{1080};
  float m_vpScaleX{1.0F}, m_vpScaleY{1.0F};

  FireModule m_fire;
  SimulationTime m_time;
  HeatMap m_heatMap; 
  bool m_showHeatMap = true;
  float m_heatMapHeight = 50.0f;
};
}  // namespace engine