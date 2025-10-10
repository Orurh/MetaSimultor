// src/ui/Editor.cc
#include "Editor.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "core/scene/Camera.h"
#include "core/scene/Scene.h"

using namespace editor;

void Editor::DrawUI(GLFWwindow* /*window*/, const engine::Camera& cam, const glm::mat4& view,
                    const glm::mat4& proj) {
  // --- минимальная панель загрузки модели ---
  ImGui::Begin("Model Loader");
  ImGui::InputText("Path", m_ModelPath, IM_ARRAYSIZE(m_ModelPath));
  ImGui::SameLine();
  if (ImGui::Button("Load")) {
    if (m_Scene)
      m_Scene->LoadPrimaryModel(m_ModelPath);
  }
  ImGui::TextUnformatted("Поддержка: любые форматы Assimp (.obj, .fbx, .dae, ...)");
  ImGui::End();

  // --- очень простой viewport без контекст-меню/дерева ---
  ImGui::Begin("Viewport");
  ImVec2 vpSize = ImGui::GetContentRegionAvail();
  SetViewportSize((int)vpSize.x, (int)vpSize.y);
  ImGui::Text("Размер вьюпорта: %dx%d", (int)vpSize.x, (int)vpSize.y);
  ImGui::End();
}
