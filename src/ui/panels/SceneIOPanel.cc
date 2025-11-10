#include "ui/panels/SceneIOPanel.h"
#include "core/Application.h"
#include "ui/EditorContext.h"
#include <imgui.h>

using namespace ui;

void SceneIOPanel::Draw(EditorContext &ctx) {
  bool open = true;
  if (!ImGui::Begin(Id(), &open)) {
    ImGui::End();
    return;
  }

  ImGui::TextUnformatted("Scene file");
  ImGui::InputText("##path", m_path.data(), m_path.size());

  ImGui::Separator();

  // Кнопки Load / Save
  if (ImGui::Button("Load")) {
    // Предполагаем, что в Scene добавлены методы: bool LoadScene(const char*
    // path);
    try {
      ctx.m_app->LoadScene(std::string(m_path.data()));
      ctx.m_sceneChanged = true;
      m_lastOk = true;
    } catch (...) {
      m_lastOk = false;
    }
    m_lastOp = "Load";
  }
  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    try {
      ctx.m_app->SaveScene(std::string(m_path.data()));
      m_lastOk = true;
    } catch (...) {
      m_lastOk = false;
    }
    m_lastOp = "Save";
  }

  // Статус
  ImGui::SameLine();
  if ((m_lastOp != nullptr) && (*m_lastOp != 0)) {
    ImGui::Text("%s: %s", m_lastOp, (m_lastOk ? "OK" : "FAILED"));
  }

  // Быстрые пресеты (опционально)
  if (ImGui::CollapsingHeader("Quick slots", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Button("data/scene1.yml")) {
      strncpy(m_path.data(), "data/scene1.yml", m_path.size());
    }
    ImGui::SameLine();
    if (ImGui::Button("data/scene2.yml")) {
      strncpy(m_path.data(), "data/scene2.yml", m_path.size());
    }
  }

  ImGui::End();
}
