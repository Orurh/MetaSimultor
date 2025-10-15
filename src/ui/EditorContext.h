#pragma once
#include <glm/glm.hpp>
#include <string>
#include "ui/EditorState.h"
#include "ui/ToolType.h"

struct GLFWwindow;
namespace engine {
class Scene;
class Camera;
class Application;
}  // namespace engine

namespace ui {
class Editor;  // <-- только форвард!

struct EditorContext {
  engine::Application* m_app = nullptr;
  GLFWwindow* m_window = nullptr;
  engine::Scene* m_scene = nullptr;
  const engine::Camera* m_camera = nullptr;

  glm::mat4 m_view{}, m_proj{};
  int m_viewportW = 1920, m_viewportH = 1080;
  int vpX{0}, vpY{0}, vpW{1920}, vpH{1080};
  float m_vpScaleX{1.F}, m_vpScaleY{1.F};

  Editor* m_editor = nullptr;  // не владеем

  // Только объявления — БЕЗ ТЕЛ в .h!
  EditorState& State();
  [[nodiscard]] const EditorState& State() const;

  [[nodiscard]] ToolType GetActiveToolType() const noexcept;

  [[nodiscard]] glm::vec2 WindowToViewportLogical(float mx, float my) const {
    return {mx - static_cast<float>(vpX), my - static_cast<float>(vpY)};
  }
  [[nodiscard]] glm::vec2 WindowToViewportFB(float mx, float my) const {
    return {(mx - static_cast<float>(vpX)) * m_vpScaleX,
            (my - static_cast<float>(vpY)) * m_vpScaleY};
  }
};
}  // namespace ui
