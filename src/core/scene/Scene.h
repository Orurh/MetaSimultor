#pragma once
#include <memory>
#include "Camera.h"
#include "configs/WindConfig.h"
#include "core/AgentManager.h"
#include "data/PrefabSystem.h"
#include "graphics/Renderer.h"
namespace engine {
class Entity;
struct ObjectModels {
  std::shared_ptr<Model> normal;   // Обычное состояние
  std::shared_ptr<Model> burning;  // Горит
  std::shared_ptr<Model> burned;   // Сгоревшее
};
struct SceneObject {
  int agent_id;             // ID агента в симуляции
  std::string object_type;  // Модель для рендеринга
  glm::vec3 position;       // Позиция (дублируется с агентом)
  glm::vec3 scale;
  bool is_visible = true;
};
class Material;

class Scene {

 public:
  Scene();
  void Init();
  void Update(float dt);

  void Render();
  void AddTree(float x, float y, const std::string& tree_type = "default_tree");
  void AddFire(float x, float y, float intensity = 1.0f) {
    m_agentManager.AddFire(x, y, intensity);
  }
  void Generate();
  void Clear();

  enum class PlaceMode { Tree, Fire };
  void SetPlaceMode(PlaceMode m) { m_placeMode = m; }
  void SetFireIntensity(float v) { m_fireIntensity = v; }
  std::vector<Agent> LoadAgentsFromGPU();

  void LoadPrimaryModel(const std::string& path);

  Camera& GetCamera() { return m_camera; }
  const Camera& GetCamera() const { return m_camera; }

  glm::mat4 GetViewMatrix() { return m_camera.GetViewMatrix(); }
  glm::mat4 GetProjMatrix(int w, int h);
  int GetVpWidth() const { return m_vpW; }
  int GetVpHeight() const { return m_vpH; }
  AgentManager& GetAgentManager() { return m_agentManager; }
  void SetViewport(int w, int h) {
    m_vpW = w;
    m_vpH = h;
  }
  void LoadObjectModels(const std::string& object_name, const std::string& normal_path,
                        const std::string& burning_path, const std::string& burned_path);
  const std::vector<SceneObject>& GetSceneObjects() const { return m_sceneObjects; }

  // Метод для добавления SceneObject
  void AddSceneObject(const SceneObject& obj) { m_sceneObjects.push_back(obj); }

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

  void AddPrefab(const std::string& prefab_id, float x, float z);
  const std::vector<std::string>& ListPrefabIds() const;

  engine::data::PrefabSystem& GetPrefabSystem() { return m_prefabSystem; }
  const engine::data::PrefabSystem& GetPrefabSystem() const { return m_prefabSystem; }
  void LoadMaterials();

  WindConfig& GetWindConfig() { return m_windConfig; }
  const WindConfig& GetWindConfig() const { return m_windConfig; }
  float GetWindTime() const { return m_windTime; }
 private:
  void RenderAgent(const Agent& agent, const SceneObject& scene_obj);
  void EnsureModelsForPrefab(const std::string& id, const engine::data::AgentPrefab& prefab);



  AgentManager m_agentManager;
  Renderer m_renderer;
  std::unordered_map<std::string, std::shared_ptr<Model>> m_loadedModels;
  std::unordered_map<std::string, std::shared_ptr<Material>> m_treeMaterials;
  std::unordered_map<std::string, std::shared_ptr<Material>> m_houseMaterials;
  std::vector<SceneObject> m_sceneObjects;
  data::PrefabSystem m_prefabSystem;
  std::unordered_map<std::string, ObjectModels> m_objectModels;
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
}  // namespace engine