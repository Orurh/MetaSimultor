#include "ui/backends/ImGuiBackend.h"
#include <imgui.h>

// clang-format off
// NOLINTBEGIN(llvm-include-order)
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// NOLINTEND(llvm-include-order)
// clang-format on

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace {
constexpr ImGuiConfigFlags Or(ImGuiConfigFlags first, ImGuiConfigFlags second) {
  return static_cast<ImGuiConfigFlags>(static_cast<unsigned>(first) |
                                       static_cast<unsigned>(second));
}
} // namespace

void ui::ImGuiBackend::Init(GLFWwindow *ui_window, const char *glsl) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags = Or(io.ConfigFlags, ImGuiConfigFlags_DockingEnable);
  io.ConfigFlags = Or(io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(ui_window, /*install_callbacks=*/true);
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

void ui::ImGuiBackend::DrawDockspace()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Хост-окно на весь экран без фона
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground; // ← важное: нет фона, сцена видна

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("##DockSpaceHost", nullptr, host_flags);
    ImGui::PopStyleVar(2);

    // Сам докспейс — с Passthru в центре
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode; // ← оставляем
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dock_flags);

    ImGui::End();
}


namespace ui {
void ImGuiBackend::SaveLayout(const char* path) {
  ImGui::SaveIniSettingsToDisk(path);
}
void ImGuiBackend::LoadLayout(const char* path) {
  ImGui::LoadIniSettingsFromDisk(path);
}
} // namespace ui