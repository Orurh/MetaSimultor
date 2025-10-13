// ui/tools/PlaceOnGroundTool.cpp
#include "ui/tools/PlaceOnGroundTool.h"
#include "math/ScreenRay.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace ui {

PlaceOnGroundTool::PlaceOnGroundTool(const char* prefabPath)
  : m_prefabPath(prefabPath) {}

void PlaceOnGroundTool::OnActivate(EditorContext&  /*ctx*/) {
  m_isActive = true;
  // // действие — подготовка ресурса
  // ctx.m_scene->EnsureModelLoaded(m_prefabPath);
}

void PlaceOnGroundTool::OnDeactivate(EditorContext&  /*ctx*/) {
  m_isActive = false;
  // // действие — очистка/сброс
}

void PlaceOnGroundTool::OnUpdate(EditorContext& ctx) {
  if (!m_isActive) { return;
}
  if (ImGui::GetIO().WantCaptureMouse) { return;
}

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    double mx = NAN;
    double my = NAN;
    glfwGetCursorPos(ctx.m_window, &mx, &my);

    glm::vec3 ro;
    glm::vec3 rd;
    math::ScreenPointToRay(static_cast<float>(mx), static_cast<float>(my),
                           static_cast<float>(ctx.m_viewportW), static_cast<float>(ctx.m_viewportH),
                           ctx.m_proj, ctx.m_view, ro, rd);

    glm::vec3 hit;
    if (math::IntersectRayPlaneY0(ro, rd, hit)) {
      // // действие — добавление сущности
      // ctx.m_scene->AddTree(hit.x, hit.z);
    }
  }
}

[[nodiscard]] ToolType PlaceOnGroundTool::Type() const noexcept {
  return ToolType::PLACE_ON_GROUND;
}
} // namespace ui
