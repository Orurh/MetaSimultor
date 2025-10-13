// src/editor/EditorExtension.cpp
#include "app/EditorExtension.h"
#include <GLFW/glfw3.h>
#include "core/Application.h"
#include "core/Input.h"
#include "imgui.h"
#include "ui/Editor.h"
#include "ui/EditorContext.h"
#include "ui/backends/ImGuiBackend.h"
#include "ui/modules/BasicToolsModule.h"

EditorExtension::~EditorExtension() noexcept = default;

void EditorExtension::OnInit(engine::Application& app) {
  m_imgui = std::make_unique<ui::ImGuiBackend>();
  m_imgui->Init(static_cast<GLFWwindow*>(app.GetNativeWindow()), "#version 330");

  m_editor = std::make_unique<ui::Editor>();
  m_editor->RegisterModule(std::make_unique<ui::BasicToolsModule>());
}

void EditorExtension::OnBeginUI(engine::Application& /*app*/, ui::EditorContext& ctx) {
  // 1) начинаем кадр ImGui
  m_imgui->BeginFrame();

  // 2) рендерим докспейс и панели (построение графа окон — до Scene::Render)
  m_imgui->DrawDockspace();
  m_editor->DrawUI(ctx);
  m_editor->TickTools(ctx);
  const ImGuiIO& io = ImGui::GetIO();
  engine::InputSystem::Instance().SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);
}

void EditorExtension::OnEndUI(engine::Application&, ui::EditorContext&) {
  // 3) завершаем кадр ImGui (Render + RenderDrawData + платформенные окна)
  m_imgui->EndFrame();
}

void EditorExtension::OnShutdown(engine::Application&) {
  m_editor.reset();  // сначала редактор
  if (m_imgui) {
    m_imgui->Shutdown();
  }
  m_imgui.reset();
}
