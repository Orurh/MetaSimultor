#pragma once
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
  // void AddFire(float x, float y) { m_agentManager.AddFire(x, y, 1.0f); }
  void AddFire(float x, float y, float intensity = 1.0f) {
      m_agentManager.AddFire(x, y, intensity);
  }
  void Generate();


  // --- интерактив ---
  enum class PlaceMode { Tree, Fire };
  void SetPlaceMode(PlaceMode m) { m_placeMode = m; }
  void SetFireIntensity(float v) { m_fireIntensity = v; }
  std::vector<Agent> LoadAgentsFromGPU();

  void LoadPrimaryModel(const std::string& path);  



    Camera&       GetCamera()       { return m_camera; }
  const Camera& GetCamera() const { return m_camera; }

  glm::mat4 GetViewMatrix()   { return m_camera.GetViewMatrix(); }
  glm::mat4 GetProjMatrix(int w, int h)  {
    // если у тебя уже есть fov/zoom в камере — используй
    return glm::perspective(m_camera.GetZoom(),
                            static_cast<float>(w) / static_cast<float>(h),
                            0.1f, 2000.0f);
  }

  void SetViewport(int w, int h) { m_vpW = w; m_vpH = h; }

private:
struct SceneObject {
    int agent_id;           // ID агента в симуляции
    std::string model_path; // Модель для рендеринга
    glm::vec3 position;     // Позиция (дублируется с агентом)
    
    bool is_visible = true;
};

  void RenderAgent(const Agent& agent);

  glm::vec3 ScreenToGround(const glm::vec2& mousePx, int w, int h);

  AgentManager m_agentManager;
  Renderer m_renderer;
  std::unordered_map<std::string, std::shared_ptr<Model>> m_loadedModels;
  std::vector<SceneObject> m_sceneObjects;
  data::PrefabSystem m_prefabSystem;
  std::shared_ptr<Model> m_treeModel;
  std::shared_ptr<Model> m_burningModel;
  std::shared_ptr<Model> m_burnedModel;
  Camera m_camera;

  // --- интерактив ---
  PlaceMode m_placeMode{PlaceMode::Tree};
  float     m_fireIntensity{1.0f};


  int m_vpW{1920}, m_vpH{1080};
};
} // namespace engine