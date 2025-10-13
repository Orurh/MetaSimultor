#pragma once
#include "ui/ToolType.h"   // enum ToolType
#include <glm/glm.hpp>

struct GLFWwindow;
namespace engine { class Scene; class Camera; }
namespace ui { class Editor; }
namespace engine { class Application; }
namespace ui {

struct EditorContext {
  
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  engine::Application* m_app = nullptr; 
  GLFWwindow*           m_window    = nullptr;   // не владеем
  engine::Scene*        m_scene     = nullptr;   // не владеем
  const engine::Camera* m_camera    = nullptr;   // не владеем
  int                   m_viewportW = 1920;
  int                   m_viewportH = 1080;
  glm::mat4             m_view{};
  glm::mat4             m_proj{};
  Editor*               m_editor      = nullptr;   // не владеем
  // NOLINTEND(misc-non-private-member-variables-in-classes)

  [[nodiscard]] ToolType GetActiveToolType() const noexcept;
};

} // namespace ui
