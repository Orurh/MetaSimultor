#include "ui/backends/ImGuiBackend.h"
#include <glm/common.hpp>
#include <imgui.h>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <cmath>

namespace {
constexpr ImGuiConfigFlags Or(ImGuiConfigFlags first, ImGuiConfigFlags second) {
  return static_cast<ImGuiConfigFlags>(static_cast<int>(first) |
                                       static_cast<int>(second));
}

float CalculateUIScale(GLFWwindow *window) {
  int width = 0;
  int height = 0;
  glfwGetWindowSize(window, &width, &height);

  float xscale = NAN;
  float yscale = NAN;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  float dpiScale = (xscale + yscale) / 2.0F;

  // Базовое разрешение для масштабирования
  const float BASE_WIDTH = 1920.0F;
  const float BASE_HEIGHT = 1080.0F;

  float scaleBySize = glm::min(static_cast<float>(width) / BASE_WIDTH,
                               static_cast<float>(height) / BASE_HEIGHT);

  float scale = scaleBySize * dpiScale;

  // округляем
  scale = glm::clamp(scale, 0.8F, 3.0F);

  if (scale < 1.1F) {
    return 1.0F;
  }
  if (scale < 1.4F) {
    return 1.25F;
  }
  if (scale < 1.7F) {
    return 1.5F;
  }
  if (scale < 2.1F) {
    return 2.0F;
  }
  return 2.5F;
}
} // namespace

void ui::ImGuiBackend::Init(GLFWwindow *ui_window, const char *glsl) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags = Or(io.ConfigFlags, ImGuiConfigFlags_DockingEnable);
  io.ConfigFlags = Or(io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);

  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();

  float uiScale = CalculateUIScale(ui_window);

  style.ScaleAllSizes(uiScale);

  ImGuiIO &io2 = ImGui::GetIO();
  io2.FontGlobalScale = uiScale;

  io2.Fonts->Clear();
  ImFontConfig fontConfig;
  fontConfig.SizePixels = 13.0F; // базовый размер
  io2.Fonts->AddFontDefault(&fontConfig);

  ImGui_ImplGlfw_InitForOpenGL(ui_window, true);
  ImGui_ImplOpenGL3_Init(glsl);
}

void ui::ImGuiBackend::Shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ui::ImGuiBackend::BeginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ui::ImGuiBackend::EndFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ui::ImGuiBackend::DrawDockspace() {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags host_flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);

  ImGui::Begin("##DockSpaceHost", nullptr, host_flags);
  ImGui::PopStyleVar(2);

  ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
  ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGui::DockSpace(dockspace_id, ImVec2(0.0F, 0.0F), dock_flags);

  ImGui::End();
}

namespace ui {
void ImGuiBackend::SaveLayout(const char *path) {
  ImGui::SaveIniSettingsToDisk(path);
}

void ImGuiBackend::LoadLayout(const char *path) {
  ImGui::LoadIniSettingsFromDisk(path);
}

void ui::ImGuiBackend::UpdateUIScale(GLFWwindow *window) {
  float newScale = CalculateUIScale(window);
  ImGuiIO &io = ImGui::GetIO();

  if (std::abs(io.FontGlobalScale - newScale) > 0.1F) {
    io.FontGlobalScale = newScale;

    ImGuiStyle &style = ImGui::GetStyle();
    static float s_prevScale = 1.0F;
    float rel = newScale / s_prevScale;
    style.ScaleAllSizes(rel);
    s_prevScale = newScale;
  }
}

} // namespace ui