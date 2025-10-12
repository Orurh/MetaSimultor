#include "ImGuiLayer.h"

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

using namespace ui;

namespace {
// Безопасная побитовая «или» для ImGuiConfigFlags (устраняет hicpp-signed-bitwise)
constexpr ImGuiConfigFlags Or(ImGuiConfigFlags a, ImGuiConfigFlags b) {
  return static_cast<ImGuiConfigFlags>(
      static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}
}  // namespace

void ui::ImGuiLayer::Init(GLFWwindow* window, const char* glsl) {
  IMGUI_CHECKVERSION();  // проверка на версию 
  ImGui::CreateContext(); // создание контекста

  ImGuiIO& io = ImGui::GetIO(); // получение ввода/вывода
  io.ConfigFlags = Or(io.ConfigFlags, ImGuiConfigFlags_DockingEnable); // разрешение докеринга 
  io.ConfigFlags = Or(io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard); // разрешение клавиатуры

  ImGui::StyleColorsDark();

  // false — не устанавливать GLFW callbacks автоматически
  ImGui_ImplGlfw_InitForOpenGL(window, /*install_callbacks=*/false); // или твой геттер
  ImGui_ImplOpenGL3_Init(glsl); // при желании можно "#version 130"
}


void ImGuiLayer::Shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::BeginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::EndFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::DrawDockspace() {
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->Pos);
  ImGui::SetNextWindowSize(vp->Size);
  ImGui::SetNextWindowViewport(vp->ID);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("DockSpace", nullptr, window_flags);
  ImGui::PopStyleVar(2);

  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("Quit", nullptr, false, false);
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  ImGui::End();
}
