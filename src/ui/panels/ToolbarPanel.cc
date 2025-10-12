// ui/panels/ToolbarPanel.cpp
#include "ui/panels/ToolbarPanel.h"
#include "core/scene/Scene.h"
#include "ui/Editor.h"
#include "core/Input.h"          // ← у тебя именно так называется инпут
#include "math/ScreenRay.h"      // луч из экрана

namespace ui {

void ToolbarPanel::Draw(EditorContext& ctx) {
  bool visible = true;
  if (!ImGui::Begin(Id(), &visible)) {
    ImGui::End();
    if (m_wasVisible) { OnHide(ctx); }
    return;
  }
  if (!m_wasVisible) { OnShow(ctx); }

  // Текущий активный инструмент
  const auto ACTIVE = ctx.GetActiveToolType();

  // Переключение инструментов
  const bool IS_PLACE = (ACTIVE == ToolType::PLACE_ON_GROUND);
  if (ImGui::Selectable("Place on ground", IS_PLACE)) {
    ctx.m_editor->SetActiveTool(ToolType::PLACE_ON_GROUND, ctx);
  }

  ImGui::Separator();
  ImGui::SliderFloat("Fire intensity", &m_fireIntensity, 0.0F, 10.0F);
  // // действие — связать с контекстом/сценой
  ctx.m_scene->SetFireIntensity(m_fireIntensity);
  ImGui::Separator();
  if (ImGui::Button("Ignite at cursor")) {
    // курсор в пикселях из GLFW
    double mx = NAN, my = NAN;
    glfwGetCursorPos(ctx.m_window, &mx, &my);

    // луч из экрана в мир (твоя утилита)
    glm::vec3 ro, rd;
    math::ScreenPointToRay(
      static_cast<float>(mx), static_cast<float>(my),
      static_cast<float>(ctx.m_viewportW), static_cast<float>(ctx.m_viewportH),
      ctx.m_proj, ctx.m_view, ro, rd);

    // пересечение с плоскостью Y=0 (твоя утилита)
    glm::vec3 hit;
    if (math::IntersectRayPlaneY0(ro, rd, hit)) {
      ctx.m_scene->AddFire(hit.x, hit.z, m_fireIntensity);
    }
  }

  ImGui::End();
}

} // namespace ui
