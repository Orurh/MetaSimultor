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
  ctx.m_scene->SetFireIntensity(m_fireIntensity);

  // --- Новый блок: поджог по фиксированным координатам (мировым)
  ImGui::Separator();
  ImGui::TextUnformatted("Ignite by world coords");
  static float fireX = 510.0f;   // X мира
  static float fireY = 501.0f;   // Z мира (у тебя второй параметр — это Z)
  ImGui::InputFloat("X", &fireX);
  ImGui::InputFloat("Y", &fireY);
  if (ImGui::Button("Ignite at (X, Y)")) {
    ctx.m_scene->AddFire(fireX, fireY, m_fireIntensity);
  }

  // --- Оставляем вариант по курсору (как было)
  ImGui::Separator();
  if (ImGui::Button("Ignite at cursor")) {
    // курсор в пикселях из GLFW
    double mx = NAN, my = NAN;
    glfwGetCursorPos(ctx.m_window, &mx, &my);

    // луч из экрана в мир
    glm::vec3 ro, rd;
    math::ScreenPointToRay(
      static_cast<float>(mx), static_cast<float>(my),
      static_cast<float>(ctx.m_viewportW), static_cast<float>(ctx.m_viewportH),
      ctx.m_proj, ctx.m_view, ro, rd);

    // пересечение с плоскостью Y=0
    glm::vec3 hit;
    if (math::IntersectRayPlaneY0(ro, rd, hit)) {
      ctx.m_scene->AddFire(hit.x, hit.z, m_fireIntensity);
    }
  }

  ImGui::End();
}


} // namespace ui
