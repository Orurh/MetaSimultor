#include "../tools/PlaceOnGroundTool.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "core/Input.h"
#include "core/scene/Scene.h"
#include "math/ScreenRay.h"
#include "ui/EditorContext.h"

namespace ui {

// ── ITool ─────────────────────────────────────────────
ToolType PlaceOnGroundTool::Type() const noexcept {
  return ToolType::PLACE_ON_GROUND;
}
const char *PlaceOnGroundTool::Name() const noexcept {
  return "Place on ground";
}
const char *PlaceOnGroundTool::Id() const noexcept {
  return "Tool.PlaceOnGround";
}

// ── Подметоды ─────────────────────────────────────────
glm::dvec2 PlaceOnGroundTool::ReadMouseWindow(GLFWwindow *win) {
  double mx = 0.0;
  double my = 0.0;
  glfwGetCursorPos(win, &mx, &my);
  return {mx, my};
}

bool PlaceOnGroundTool::WindowToViewportLogical(const EditorContext &ctx,
                                                const glm::dvec2 &mouseWin,
                                                glm::vec2 &outVpLogical) {
  const glm::vec2 VP_L = ctx.WindowToViewportLogical(
      static_cast<float>(mouseWin.x), static_cast<float>(mouseWin.y));
  const bool INSIDE =
      (VP_L.x >= 0.F && VP_L.y >= 0.F && VP_L.x < static_cast<float>(ctx.vpW) &&
       VP_L.y < static_cast<float>(ctx.vpH));
  if (INSIDE) {
    outVpLogical = VP_L;
  }
  return INSIDE;
}

glm::vec2 PlaceOnGroundTool::ToFramebufferPx(const EditorContext &ctx,
                                             const glm::vec2 &vpLogical) {
  return {vpLogical.x * ctx.m_vpScaleX, vpLogical.y * ctx.m_vpScaleY};
}

void PlaceOnGroundTool::BuildScreenRay(const EditorContext &ctx,
                                       const glm::vec2 &px,
                                       glm::vec3 &outOrigin,
                                       glm::vec3 &outDir) {
  const auto FB_W = static_cast<float>(ctx.m_viewportW);
  const auto FB_H = static_cast<float>(ctx.m_viewportH);
  math::ScreenPointToRay(px.x, px.y, FB_W, FB_H, ctx.m_proj, ctx.m_view,
                         outOrigin, outDir);
}

bool PlaceOnGroundTool::RaycastPlaneY0(const glm::vec3 &ro, const glm::vec3 &rd,
                                       glm::vec3 &outHit) {
  return math::IntersectRayPlaneY0(ro, rd, outHit);
}

bool PlaceOnGroundTool::RaycastTerrain(const engine::Terrain &T,
                                       const glm::vec3 &ro, const glm::vec3 &rd,
                                       float anchorY, glm::vec3 &outHit) {
  auto f = [&](float t) -> float {
    const glm::vec3 p = ro + rd * t;
    return p.y - (T.GetHeightAt(p.x, p.z) + anchorY);
  };

  const float T_MAX = 5000.F;
  const float STEP = 2.F;

  float t0 = 0.F;
  float v0 = f(t0);

  for (float t = STEP; t <= T_MAX; t += STEP) {
    float v1 = f(t);
    const bool SIGN_CHANGE = (v0 > 0.F && v1 <= 0.F) || (v0 < 0.F && v1 >= 0.F);
    if (SIGN_CHANGE) {
      float a = t - STEP;
      float b = t;
      for (int i = 0; i < 24; ++i) {
        float m = 0.5F * (a + b);
        float vm = f(m);
        if ((v0 > 0.F && vm > 0.F) || (v0 < 0.F && vm < 0.F)) {
          a = m;
          v0 = vm;
        } else {
          b = m;
        }
      }
      const float THIT = 0.5F * (a + b);
      outHit = ro + rd * THIT;
      return true;
    }
    t0 = t;
    v0 = v1;
  }
  return false;
}

// ── Основной тик ──────────────────────────────────────
void PlaceOnGroundTool::OnUpdate(EditorContext &ctx) {
  if (engine::InputSystem::Instance().UIMouseCaptured()) {
    return;
  }
  // игнорируем, если нажата клавиша Ctrl
  if (glfwGetKey(ctx.m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(ctx.m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
    return;
  }
  if (!engine::InputSystem::Instance().GetMouseButton(
          engine::MouseButton::Left, engine::InputState::Pressed)) {
    return;
  }
  if (!ctx.m_scene) return;

  // 1) курсор окна
  const glm::dvec2 MOUSE_WIN = ReadMouseWindow(ctx.m_window);

  // 2) окно → панель (логические px)
  glm::vec2 vpLogical{};
  const bool INSIDE = WindowToViewportLogical(ctx, MOUSE_WIN, vpLogical);

  if (!INSIDE) {
    return;
  }

  // 3) логические → framebuffer px
  const glm::vec2 PX = ToFramebufferPx(ctx, vpLogical);

  // 4) луч из пикселя FB
  glm::vec3 ro{};
  glm::vec3 rd{};
  BuildScreenRay(ctx, PX, ro, rd);

  // 5) сначала — попытка удаления (ПКМ или Space+ЛКМ)
  if (HandleDeleteClick(ctx, ro, rd))
    return;

  // 6) далее — ИСХОДНАЯ логика: плейсмент только при ЛКМ
  if (!engine::InputSystem::Instance().GetMouseButton(
          engine::MouseButton::Left, engine::InputState::Pressed))
    return;

  glm::vec3 hitTerrain{};
  bool hitT = false;
  float anchorY = 0.0f;

  auto &state = ctx.State();
  using PlaceKind = EditorState::PlaceKind;

  if (ctx.m_scene->GetTerrain() != nullptr) {
    // Якорь только для префаба, для огня/IGNITE anchorY = 0
    if (state.m_placeKind == PlaceKind::PREFAB &&
        !state.m_selectedPrefab.empty()) {
      const auto &prefab = ctx.m_scene->GetPrefabSystem().GetPrefab(
          state.m_selectedPrefab);
      anchorY = -prefab.m_bounds_min.y * prefab.m_modelScale.y;
    }

    hitT = RaycastTerrain(*ctx.m_scene->GetTerrain(), ro, rd, anchorY, hitTerrain);
  }

  if (hitT) {
    switch (state.m_placeKind) {
      case PlaceKind::FIRE: {
        // гарантируем, что огонь стоит ровно на земле
        hitTerrain.y = ctx.m_scene->GetGroundHeight(hitTerrain.x, hitTerrain.z);
        ctx.m_scene->AddFireAt(hitTerrain, state.m_fireIntensity);
        break;
      }
      case PlaceKind::PREFAB: {
        ctx.m_scene->AddPrefab(state.m_selectedPrefab, hitTerrain.x,
                               hitTerrain.z, 1.0f);
        break;
      }
      case PlaceKind::IGNITE: {
        // поджечь ближайший объект к точке попадания
        // используем тот же радиус, что и для RemoveNearestAt (kPickRadiusXZ)
        ctx.m_scene->IgniteNearest(hitTerrain, kPickRadiusXZ);
        break;
      }
    }
    return;
  }

  // 6b) фолбек: Y=0 (как у вас было)
  glm::vec3 hitPlane{}; 
  if (!RaycastPlaneY0(ro, rd, hitPlane)) {
    return;
  }

  switch (state.m_placeKind) {
    case PlaceKind::FIRE: {
      ctx.m_scene->AddFire(hitPlane.x, hitPlane.z, state.m_fireIntensity);
      break;
    }
    case PlaceKind::PREFAB: {
      ctx.m_scene->AddPrefab(state.m_selectedPrefab, hitPlane.x, hitPlane.z,
                             1.0f);
      break;
    }
    case PlaceKind::IGNITE: {
      ctx.m_scene->IgniteNearest(hitPlane, kPickRadiusXZ);
      break;
    }
  }
}


bool PlaceOnGroundTool::IsDeleteGesture() noexcept {
  using engine::InputState;
  using engine::InputSystem;
  using engine::KeyCode;
  using engine::MouseButton;

  const bool RMB_PRESSED = InputSystem::Instance().GetMouseButton(
      MouseButton::Right, InputState::Pressed);
  const bool SPACE_LMB_PRESSED =
      InputSystem::Instance().GetKey(KeyCode::Space) &&
      InputSystem::Instance().GetMouseButton(MouseButton::Left,
                                             InputState::Pressed);
  return RMB_PRESSED || SPACE_LMB_PRESSED;
}

// ── Обработчик удаления ────────────────────────────
bool PlaceOnGroundTool::HandleDeleteClick(EditorContext &ctx,
                                          const glm::vec3 &ro,
                                          const glm::vec3 &rd) {
  if (!IsDeleteGesture()) {
    return false;
  }

  glm::vec3 hit{};
  bool got = false;

  // Предпочтительно — точка на террейне
  if (ctx.m_scene->GetTerrain() != nullptr) {
    got = RaycastTerrain(*ctx.m_scene->GetTerrain(), ro, rd, /*anchorY=*/0.0f,
                         hit);
  }
  // Фолбек — плоскость Y=0
  if (!got) {
    got = RaycastPlaneY0(ro, rd, hit);
  }
  if (!got)
    return true; // жест был, но по лучу ничего не попали — считать обработанным

  // Удаляем ближайший объект (или огонь) рядом с hit.xz
  // ВАЖНО: метод сцены должен существовать (см. нашу предыдущую заготовку
  // RemoveNearestAt)
  ctx.m_scene->RemoveNearestAt(hit, kPickRadiusXZ);
  return true;
}

} // namespace ui
