#pragma once
#include "ui/ToolType.h"
#include <glm/glm.hpp>

struct GLFWwindow;
namespace engine { class Scene; class Camera; class Application; }
namespace ui { class Editor; }

namespace ui {

struct EditorContext {
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  engine::Application* m_app      = nullptr;
  GLFWwindow*          m_window   = nullptr;   // не владеем
  engine::Scene*       m_scene    = nullptr;   // не владеем
  const engine::Camera* m_camera  = nullptr;   // не владеем

  // Матрицы камеры
  glm::mat4 m_view{};
  glm::mat4 m_proj{};

  // Размеры вьюпорта в framebuffer-пикселях (для Ray/Projection)
  int m_viewportW = 1920;
  int m_viewportH = 1080;

  // --- NEW: прямоугольник рабочей области (логические пиксели ImGui)
  int vpX{0}, vpY{0}, vpW{1920}, vpH{1080};

  // --- NEW: HiDPI scale (логические → framebuffer)
  float vpScaleX{1.f}, vpScaleY{1.f};

  Editor* m_editor = nullptr; // не владеем
  // NOLINTEND

  [[nodiscard]] ToolType GetActiveToolType() const noexcept;

  // --- NEW: перевод координаты окна → координаты вьюпорта в framebuffer-пикселях
glm::vec2 WindowToViewportLogical(float mx, float my) const {
  return { mx - float(vpX), my - float(vpY) };
}

// окно -> внутрь вьюпорта (framebuffer-пиксели)
glm::vec2 WindowToViewportFB(float mx, float my) const {
  return {
    (mx - float(vpX)) * vpScaleX,
    (my - float(vpY)) * vpScaleY
  };
}
};

} // namespace ui
