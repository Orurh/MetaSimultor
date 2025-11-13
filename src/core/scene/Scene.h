#pragma once
#include "Camera.h"
#include "Terrain.h"
#include "configs/WindConfig.h"
#include "core/AgentManager.h"
#include "data/PrefabSystem.h"
#include <memory>

#include "sceneObj/ObjectModelStore.h"
#include "sceneObj/SceneObjectStore.h"
namespace engine {
class Model;
class Entity;
class Material;

class Scene {

public:
  Scene();
  void Init();
  void Update(float dt);

  void Render();
  void AddTree(float x, float z, const std::string &tree_type = "default_tree");
  void AddFire(float x, float z, float intensity = 1.0F);

  void AddFireAt(const glm::vec3 &world, float intensity = 1.0F);

  void Generate();
  void Clear();
  void Shutdown();
  enum class PlaceMode { Tree, Fire };
  void SetPlaceMode(PlaceMode m) { m_placeMode = m; }
  void SetFireIntensity(float v) { m_fireIntensity = v; }
  std::vector<Agent> LoadAgentsFromGPU();

  void LoadPrimaryModel(const std::string &path);

  Camera &GetCamera() { return m_camera; }
  const Camera &GetCamera() const { return m_camera; }

  glm::mat4 GetViewMatrix() { return m_camera.GetViewMatrix(); }
  glm::mat4 GetProjMatrix(int w, int h);
  int GetVpWidth() const { return m_vpW; }
  int GetVpHeight() const { return m_vpH; }
  AgentManager &GetAgentManager() { return m_agentManager; }
  void SetViewport(int w, int h) {
    m_vpW = w;
    m_vpH = h;
  }
  void LoadObjectModels(const std::string &object_name,
                        const std::string &normal_path,
                        const std::string &burning_path,
                        const std::string &burned_path);

  const std::vector<SceneObject> &GetSceneObjects() const {
    return m_sceneObjects.All();
  }

  // Метод для добавления SceneObject
  void AddSceneObject(const SceneObject &obj) { m_sceneObjects.Add(obj); }

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

  PlaceMode GetPlaceMode() const { return m_placeMode; }

  void AddPrefab(const std::string &prefab_id, float x, float z);
  bool RemoveNearestAt(const glm::vec3 &world, float maxXZDist);

  const std::vector<std::string> &ListPrefabIds() const;

  engine::data::PrefabSystem &GetPrefabSystem() { return m_prefabSystem; }
  const engine::data::PrefabSystem &GetPrefabSystem() const {
    return m_prefabSystem;
  }

  WindConfig &GetWindConfig() { return m_windConfig; }
  const WindConfig &GetWindConfig() const { return m_windConfig; }
  float GetWindTime() const { return m_windTime; }
  void ReloadModelForPrefab(const std::string &prefab_id,
                            const engine::data::AgentPrefab &prefab);
  void AddSceneObjectFromSerialized(const SceneObject &obj);
  void GenerateTerrain(const TerrainGenerationSettings &s);

  Terrain *GetTerrain() { return m_terrain.get(); }
  const Terrain *GetTerrain() const { return m_terrain.get(); }

  float GetGroundHeight(float x, float z) const;

private:
  void RenderAgent(const Agent &agent, const SceneObject &scene_obj);
  void EnsureModelsForPrefab(const std::string &id,
                             const engine::data::AgentPrefab &prefab);

  // для удаления префабов
  int FindClosestObjectXZ(const glm::vec3 &world, float maxXZDist) const;
  void RemoveSceneObjectByIndex(size_t idx);

  AgentManager m_agentManager;
  std::unique_ptr<Terrain> m_terrain;
  data::PrefabSystem m_prefabSystem;

  SceneObjectStore m_sceneObjects;
  ObjectModelStore m_objectModels;

  Camera m_camera;
  WindConfig m_windConfig;
  float m_windTime = 0.0F;
  // --- интерактив ---
  PlaceMode m_placeMode{PlaceMode::Tree};
  float m_fireIntensity{1.0F};

  int m_vpX{0}, m_vpY{0};
  int m_vpW{1920}, m_vpH{1080};
  float m_vpScaleX{1.0F}, m_vpScaleY{1.0F};
};
} // namespace engine