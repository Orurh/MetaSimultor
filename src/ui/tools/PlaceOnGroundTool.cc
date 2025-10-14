#include "ui/tools/PlaceOnGroundTool.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>                    // + нужно для std::abs

#include "core/Input.h"
#include "core/scene/Scene.h"
#include "ui/EditorContext.h"
#include "math/ScreenRay.h"  
#include <iostream>
using engine::MouseButton;
using engine::InputState;

namespace ui {

ToolType PlaceOnGroundTool::Type() const noexcept { return ToolType::PLACE_ON_GROUND; }
const char* PlaceOnGroundTool::Name() const noexcept { return "Place on ground"; }
const char* PlaceOnGroundTool::Id()   const noexcept { return "Tool.PlaceOnGround"; }

void PlaceOnGroundTool::OnUpdate(EditorContext& ctx) {
  if (engine::InputSystem::Instance().UIMouseCaptured()) return;
  if (!engine::InputSystem::Instance().GetMouseButton(engine::MouseButton::Left,
                                                      engine::InputState::Pressed)) return;

  // 1) Курсор в логических координатах окна
  double mx = 0.0, my = 0.0;
  glfwGetCursorPos(ctx.m_window, &mx, &my);

  // 2) Окно -> панель вьюпорта (логические пиксели панели)
  const glm::vec2 vpl = ctx.WindowToViewportLogical((float)mx, (float)my);
  if (vpl.x < 0.f || vpl.y < 0.f || vpl.x >= (float)ctx.vpW || vpl.y >= (float)ctx.vpH) return;

  // 3) Логические -> framebuffer пиксели (HiDPI масштаб панели)
  const float px = vpl.x * ctx.vpScaleX;   // 0 слева
  const float py = vpl.y * ctx.vpScaleY;   // 0 СВЕРХУ (ровно как ждёт ScreenPointToRay)

  // 4) Размеры framebuffer рендер-области (под них построена проекция)
  const float fbW = (float)ctx.m_viewportW;  // = vpW * vpScaleX
  const float fbH = (float)ctx.m_viewportH;  // = vpH * vpScaleY

  // 5) Луч из пикселя экрана -> мир
  glm::vec3 ro{}, rd{};
  math::ScreenPointToRay(px, py, fbW, fbH, ctx.m_proj, ctx.m_view, ro, rd);

  // 6) Пересечение с плоскостью Y=0
  glm::vec3 hit{};
  if (!math::IntersectRayPlaneY0(ro, rd, hit)) return;

  // (необязательно) отладка — убедиться, что всё консистентно
  // std::cout << "[DBG] win=("<<mx<<","<<my<<") vpLog=("<<vpl.x<<","<<vpl.y
  //           <<") fb=("<<px<<","<<py<<") fbSize=("<<fbW<<","<<fbH<<") hit=("
  //           <<hit.x<<","<<hit.y<<","<<hit.z<<")\n";

  // 7) Действие
  if (ctx.m_scene->GetPlaceMode() == engine::Scene::PlaceMode::Tree)
    ctx.m_scene->AddTree(hit.x, hit.z);
  else
    ctx.m_scene->AddFire(hit.x, hit.z);
}











} // namespace ui   