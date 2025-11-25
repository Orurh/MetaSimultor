// src/editor/EditorExtension.cpp
#include "EditorExtension.h"
#include "core/Application.h"
#include "core/Input.h"
#include "core/scene/Scene.h"
#include "imgui.h"
#include "ui/Editor.h"
#include "ui/EditorContext.h"
#include "ui/backends/ImGuiBackend.h"
#include "ui/modules/FireUIModule.h"
#include "ui/modules/PrefabModule.h"
#include "ui/modules/RecorderModule.h"
#include "ui/modules/SceneModule.h"
#include "ui/modules/TerrainModule.h"
#include <GLFW/glfw3.h>
EditorExtension::~EditorExtension() noexcept = default;

void EditorExtension::OnInit(engine::Application &app) {
  m_imgui = std::make_unique<ui::ImGuiBackend>();
  m_imgui->Init(app.GetNativeWindow(), "#version 330");

  m_editor = std::make_unique<ui::Editor>();
  m_editor->RegisterModule(std::make_unique<ui::SceneModule>());
  m_editor->RegisterModule(std::make_unique<ui::TerrainModule>());
  m_editor->RegisterModule(std::make_unique<ui::PrefabModule>());
  m_editor->RegisterModule(std::make_unique<ui::FireUIModule>());
  m_editor->RegisterModule(std::make_unique<ui::RecorderModule>());

  ui::EditorContext ec{};
  ec.m_app = &app;
  ec.m_window = app.GetNativeWindow();
  ec.m_scene = &app.GetScene();
  m_editor->SetActiveTool(ui::ToolType::PLACE_ON_GROUND, ec);
}

void EditorExtension::OnBeginUI(engine::Application & /*app*/, ui::EditorContext &ctx) {
  // 1) начинаем кадр ImGui
  m_imgui->BeginFrame();
  m_imgui->DrawDockspace();

  // 2) Получаем актуальный viewport из ImGui
  ImGuiViewport *vp = ImGui::GetMainViewport();
  ctx.vpX = static_cast<int>(vp->WorkPos.x);
  ctx.vpY = static_cast<int>(vp->WorkPos.y);
  ctx.vpW = static_cast<int>(vp->WorkSize.x);
  ctx.vpH = static_cast<int>(vp->WorkSize.y);

  // 3) HiDPI scale
  int fbw = 0, fbh = 0, ww = 0, wh = 0;
  glfwGetFramebufferSize(ctx.m_window, &fbw, &fbh);
  glfwGetWindowSize(ctx.m_window, &ww, &wh);
  ctx.m_vpScaleX = (ww > 0) ? static_cast<float>(fbw) / static_cast<float>(ww) : 1.f;
  ctx.m_vpScaleY = (wh > 0) ? static_cast<float>(fbh) / static_cast<float>(wh) : 1.f;

  // 4) Размеры вьюпорта для проекций (в framebuffer-пикселях)
  ctx.m_viewportW = static_cast<int>(ctx.vpW * ctx.m_vpScaleX);
  ctx.m_viewportH = static_cast<int>(ctx.vpH * ctx.m_vpScaleY);

  // 5) Обновляем матрицы из сцены с ПРАВИЛЬНЫМИ размерами
  ctx.m_view = ctx.m_scene->GetViewMatrix();
  ctx.m_proj = ctx.m_scene->GetProjMatrix(ctx.m_viewportW, ctx.m_viewportH);

  // 6) Сообщаем сцене актуальные параметры viewport
  ctx.m_scene->SetViewportRect(ctx.vpX, ctx.vpY, ctx.vpW, ctx.vpH);
  ctx.m_scene->SetViewportScale(ctx.m_vpScaleX, ctx.m_vpScaleY);

  // 7) Рендер UI редактора
  m_editor->DrawUI(ctx);

  // 8) Обновляем состояние захвата ввода
  const ImGuiIO &io = ImGui::GetIO();
  engine::InputSystem::Instance().SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

  // 9) Обновляем активные инструменты
  m_editor->TickTools(ctx);
}

void EditorExtension::OnEndUI(engine::Application &, ui::EditorContext &) {
  // 3) завершаем кадр ImGui (Render + RenderDrawData + платформенные окна)
  m_imgui->EndFrame();
}

void EditorExtension::OnShutdown(engine::Application &) {
  m_editor.reset();  // сначала редактор
  if (m_imgui) {
    m_imgui->Shutdown();
  }
  m_imgui.reset();
}
