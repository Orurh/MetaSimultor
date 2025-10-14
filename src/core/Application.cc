// Application.cc
#include "Application.h"
#include "app/IAppExtension.h"
#include "ui/EditorContext.h"

// clang-format off
// NOLINTBEGIN(llvm-include-order)
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// NOLINTEND(llvm-include-order)
// clang-format on

// ImGui core + backends

#include <core/Window.h>
#include <core/scene/Scene.h>
#include "data/SceneSerializer.h"
#include <iostream>
#include <stdexcept>
#include <thread>

namespace engine {
Application::Application(int width, int height, const char *title)
    : m_last_frame_time(static_cast<float>(glfwGetTime())), m_width(width),
      m_height(height) {
  // Создание окна/GL
  m_window = std::make_unique<Window>(width, height, std::string(title));
  InitGL(); 

  // Сцена
  m_scene = std::make_unique<Scene>();
  m_scene->Init();
}

engine::Application::~Application() = default; 

void Application::AddExtension(std::unique_ptr<IAppExtension> ext) {
  m_extensions.push_back(std::move(ext));
}

void Application::Run() {
  for (auto& e : m_extensions) e->OnInit(*this);

  while (!ShouldClose()) {
    TickCore();

    // Собираем контекст на кадр (размеры, матрицы и т.п.)
    ui::EditorContext ctx{};
    ctx.m_editor    = &m_editor; 
    ctx.m_app      = this;  
    ctx.m_window    = GetNativeWindow();
    ctx.m_scene     = m_scene.get();
    ctx.m_viewportW = GetViewportWidth();
    ctx.m_viewportH = GetViewportHeight();
    ctx.m_view      = m_scene->GetViewMatrix();
    ctx.m_proj      = m_scene->GetProjMatrix(ctx.m_viewportW, ctx.m_viewportH);

    // === ВАЖНО: сначала начинаем UI-кадр (NewFrame + построение окон) ===
    for (auto& e : m_extensions) e->OnBeginUI(*this, ctx);
    // Теперь ImGui::GetIO().WantCaptureMouse/Keyboard — АКТУАЛЬНЫЕ
    UpdateScene();
    RenderScene();

    // Завершаем UI (Render + RenderDrawData)
    for (auto& e : m_extensions) e->OnEndUI(*this, ctx);

    SwapBuffers();
  }

  for (auto& e : m_extensions) e->OnShutdown(*this);
}


void Application::InitGL() {
  // активировать GL-контекст окна (если нужно), инициализировать glad
  auto *win = static_cast<GLFWwindow *>(GetNativeWindow());
  glfwMakeContextCurrent(win);

  if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
    throw std::runtime_error("Failed to initialize GLAD");
  }
  glViewport(0, 0, m_width, m_height);
}

void Application::TickCore() {
  // dt
  auto now = static_cast<float>(glfwGetTime());
  m_dt = now - m_last_frame_time;
  m_last_frame_time = now;

  // события окна/инпут
  m_window->PollEvents();
}

void Application::UpdateScene() { m_scene->Update(m_dt); }

void Application::RenderScene() { m_scene->Render(); }

void Application::SwapBuffers() { m_window->SwapBuffers(); }

bool Application::ShouldClose() const { return m_window->ShouldClose(); }

GLFWwindow* engine::Application::GetNativeWindow() const {
  return m_window->GetWindow();
}

int Application::GetViewportWidth() const { return m_width; }
int Application::GetViewportHeight() const { return m_height; }
float Application::GetDeltaTime() const { return m_dt; }
Scene &Application::GetScene() { return *m_scene; }

bool Application::SaveScene(const std::string& path) {
    if (!m_serializer) {
        m_serializer = std::make_unique<data::SceneSerializer>(m_scene);
    }
    return m_serializer->Serialize(path);
}

void Application::LoadScene(const std::string& path) {
    if (!m_serializer) {
        m_serializer = std::make_unique<data::SceneSerializer>(
           m_scene);
    }
    m_serializer->Deserialize(path);
}
} // namespace engine