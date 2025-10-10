#pragma once
#include "Components.h"
#include "graphics/Renderer.h"
#include "core/AgentManager.h"
#include "Camera.h"
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

  void LoadPrimaryModel(const std::string& path);  
private:
  std::vector<Agent> LoadAgentsFromGPU();
  void RenderAgent(const Agent& agent);

  glm::vec3 ScreenToGround(const glm::vec2& mousePx, int w, int h);

  AgentManager m_agentManager;
  Renderer m_renderer;
  std::shared_ptr<Model> m_treeModel;
  std::shared_ptr<Model> m_burningModel;
  std::shared_ptr<Model> m_burnedModel;
  Camera m_camera;

  // --- интерактив ---
  PlaceMode m_placeMode{PlaceMode::Tree};
  float     m_fireIntensity{1.0f};
};
} // namespace engine