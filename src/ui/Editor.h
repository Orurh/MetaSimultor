// src/ui/Editor.h
#pragma once
#include <glm/glm.hpp>
#include <string>          // NEW
struct GLFWwindow;
namespace engine { class Scene; class Camera; }

namespace editor {
  
class Editor {
 public:
  explicit Editor(engine::Scene* scene) : m_Scene(scene) {}
  void SetViewportSize(int w, int h) { m_ViewportW = w; m_ViewportH = h; }
  void DrawUI(GLFWwindow* window, const engine::Camera& cam,
              const glm::mat4& view, const glm::mat4& proj);

 private:
  void DrawToolbar();
  void DrawHierarchy();
  void DrawInspector();
  void SceneContextMenu(const engine::Camera& cam, const glm::mat4& view, const glm::mat4& proj);

 private:
  engine::Scene* m_Scene = nullptr;
  int  m_ViewportW = 1920, m_ViewportH = 1080;
  bool m_AddTreeMode = false;                // по умолчанию выкл в минимальном режиме
  char m_ModelPath[512] = "res/models/LowPolytree/CartoonTree.obj"; // NEW
};
}
