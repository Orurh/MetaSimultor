#include "ui/tools/PlaceOnGroundTool.h"

#include <GLFW/glfw3.h>
#include <cmath>  // + нужно для std::abs
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <iostream>
#include "core/Input.h"
#include "core/scene/Scene.h"
#include "math/ScreenRay.h"
#include "ui/EditorContext.h"

namespace ui {

ToolType PlaceOnGroundTool::Type() const noexcept {
  return ToolType::PLACE_ON_GROUND;
}
const char* PlaceOnGroundTool::Name() const noexcept {
  return "Place on ground";
}
const char* PlaceOnGroundTool::Id() const noexcept {
  return "Tool.PlaceOnGround";
}

void PlaceOnGroundTool::OnUpdate(EditorContext& ctx) {
  if (engine::InputSystem::Instance().UIMouseCaptured()) {
    return;
  }
  if (!engine::InputSystem::Instance().GetMouseButton(engine::MouseButton::Left,
                                                      engine::InputState::Pressed)) {
    return;
  }

  // 1) Курсор в логических координатах окна
  double mx = 0.0;
  double my = 0.0;
  glfwGetCursorPos(ctx.m_window, &mx, &my);

  // 2) Окно -> панель вьюпорта (логические пиксели панели)
  const glm::vec2 VPL = ctx.WindowToViewportLogical(static_cast<float>(mx), static_cast<float>(my));
  if (VPL.x < 0.F || VPL.y < 0.F || VPL.x >= static_cast<float>(ctx.vpW) ||
      VPL.y >= static_cast<float>(ctx.vpH)) {
    return;
  }

  // 3) Логические -> framebuffer пиксели (HiDPI масштаб панели)
  const float PX = VPL.x * ctx.m_vpScaleX;
  const float PY = VPL.y * ctx.m_vpScaleY;

  // 4) Размеры framebuffer рендер-области (под них построена проекция)
  const auto FB_W = static_cast<float>(ctx.m_viewportW);
  const auto FB_H = static_cast<float>(ctx.m_viewportH);

  // 5) Луч из пикселя экрана -> мир
  glm::vec3 ro{};
  glm::vec3 rd{};
  math::ScreenPointToRay(PX, PY, FB_W, FB_H, ctx.m_proj, ctx.m_view, ro, rd);

  // 6) Пересечение с плоскостью Y=0
  glm::vec3 hit{};
  if (!math::IntersectRayPlaneY0(ro, rd, hit)) {
    return;
  }

  // 7) Действие
  if (ctx.State().m_placeKind == EditorState::PlaceKind::FIRE) {
    ctx.m_scene->AddFire(hit.x, hit.z, ctx.State().m_fireIntensity);
  } else {
    ctx.m_scene->AddPrefab(ctx.State().m_selectedPrefab, hit.x, hit.z);
  }
}
}  // namespace ui