#include "ui/panels/PrefabBrowserPanel.h"
#include "core/agent.h"
#include "core/scene/Scene.h"
#include "data/PrefabSystem.h"
#include "graphics/Material.h"
#include "ui/EditorContext.h"
#include "ui/panels/PrefabPreviewRenderer.h"
#include <filesystem>
#include <imgui.h>
#include <memory>
using engine::data::AgentPrefab;

namespace ui {

inline bool DragVec3(const char *label, glm::vec3 &v, float speed = 0.1F,
                     float v_min = -FLT_MAX, float v_max = FLT_MAX,
                     const char *fmt = "%.3f") {
  float a[3] = {v.x, v.y, v.z};
  const bool CHANGED = ImGui::DragFloat3(label, a, speed, v_min, v_max, fmt);
  if (CHANGED) {
    v.x = a[0];
    v.y = a[1];
    v.z = a[2];
  }
  return CHANGED;
}

void PrefabBrowserPanel::OnShow(EditorContext &ctx) {

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PrefabBrowserPanel::PrefabBrowserPanel()
    : m_preview(std::make_unique<PrefabPreviewRenderer>(300, 300)) {}

PrefabBrowserPanel::~PrefabBrowserPanel() {}
void PrefabBrowserPanel::OnHide(EditorContext &ctx) {}
void PrefabBrowserPanel::Draw(EditorContext &ctx) {
  bool open = true;
  if (!ImGui::Begin(Id(), &open)) {
    ImGui::End();
    return;
  }

  const auto &ids = ctx.m_scene->ListPrefabIds();
  if (ids.empty()) {
    ImGui::TextDisabled("No prefabs loaded");
    ImGui::End();
    return;
  }

  const std::string &id = EnsureSelection(ctx, ids);
  AgentPrefab &P = *m_edit;

  ImGui::Columns(3, "PrefabBrowserColumns", true);

  ImGui::BeginChild("LeftColumn", ImVec2(0, 0), true);
  DrawLeftColumn(ctx, ids);
  ImGui::EndChild();

  ImGui::NextColumn();

  ImGui::BeginChild("CenterColumn", ImVec2(0, 0), true);

  ImGui::SeparatorText("Model Preview");

  ImGui::BeginChild("PreviewArea", ImVec2(0, 300 + 40), true,
                    ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse);
  DrawModelPreview(ctx);
  ImGui::Separator();
  ImGui::TextDisabled("LMB + drag: rotate");
  ImGui::TextDisabled("Mouse wheel: zoom");
  ImGui::TextDisabled("Yaw: %.1f, Pitch: %.1f", m_previewYaw, m_previewPitch);

  ImGui::EndChild();
  ImGui::EndChild();
  ImGui::NextColumn();

  ImGui::BeginChild("RightColumn", ImVec2(0, 0), true);

  DrawHeader(id);
  DrawType(P);
  DrawModelSection(P);
  DrawBounds(P);
  DrawPhysics(P);
  DrawFireSection(P);
  DrawFirePoints(P);
  DrawVisuals(P);

  ImGui::Separator();
  DrawSaveButtons(ctx, id, P);
  DrawSavePopup(ctx, id, P);

  ImGui::EndChild();

  ImGui::Columns(1);

  ImGui::End();
}

void PrefabBrowserPanel::DrawModelPreview(EditorContext &ctx) {
  AgentPrefab &P = *m_edit;

  if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    ImVec2 delta = ImGui::GetMouseDragDelta(0, 0.0f);
    if (!m_dragging) {
      m_lastMouse = ImGui::GetMousePos();
      m_dragging = true;
    }
    ImVec2 d = ImVec2(ImGui::GetMousePos().x - m_lastMouse.x,
                      ImGui::GetMousePos().y - m_lastMouse.y);
    m_preview->Rotate(d.x * 0.2f, d.y * 0.2f);
    m_lastMouse = ImGui::GetMousePos();
  } else {
    m_dragging = false;
  }

  if (ImGui::IsWindowHovered()) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0) {
      m_preview->Zoom(wheel * 2.0f);
      ImGui::GetIO().MouseWheel = 0;
    }
  }

  m_preview->Render(P);

  ImGui::Image((ImTextureID)(intptr_t)m_preview->GetTexture(), ImVec2(300, 300),
               ImVec2(0, 1), ImVec2(1, 0));
}

// ------------------ helpers ------------------

const std::string &
PrefabBrowserPanel::EnsureSelection(EditorContext &ctx,
                                    const std::vector<std::string> &ids) {
  if (m_selectedIndex < 0 || m_selectedIndex >= (int)ids.size()) {
    m_selectedIndex = 0;
    m_edit.reset();
  }
  const std::string &id = ids[m_selectedIndex];
  if (!m_edit.has_value()) {
    m_edit = ctx.m_scene->GetPrefabSystem().GetPrefab(id);
    // Синхронизируем текстовые буферы один раз при загрузке
    std::snprintf(m_modelBuf, sizeof(m_modelBuf), "%s",
                  m_edit->m_modelPath.c_str());
    std::snprintf(m_matNormalBuf, sizeof(m_matNormalBuf), "%s",
                  m_edit->normal_material.normal_path.c_str());
    std::snprintf(m_matBurningBuf, sizeof(m_matBurningBuf), "%s",
                  m_edit->normal_material.specular_path.c_str());
    std::snprintf(m_matBurnedBuf, sizeof(m_matBurnedBuf), "%s",
                  m_edit->normal_material.diffuse_path.c_str());
    std::snprintf(m_fireMaterialBuf, sizeof(m_fireMaterialBuf), "%s",
                 m_edit->m_fireMaterialId.c_str());
  }
  return id;
}

void PrefabBrowserPanel::DrawLeftColumn(EditorContext &ctx,
                                        const std::vector<std::string> &ids) {
  for (int i = 0; i < (int)ids.size(); ++i) {
    const bool sel = (i == m_selectedIndex);
    if (ImGui::Selectable(ids[i].c_str(), sel)) {
      m_selectedIndex = i;
      // Перезагружаем редактируемую копию и буферы
      m_edit = ctx.m_scene->GetPrefabSystem().GetPrefab(ids[i]);
      std::snprintf(m_modelBuf, sizeof(m_modelBuf), "%s",
                    m_edit->m_modelPath.c_str());
      std::snprintf(m_matNormalBuf, sizeof(m_matNormalBuf), "%s",
                    m_edit->normal_material.normal_path.c_str());
      std::snprintf(m_matBurningBuf, sizeof(m_matBurningBuf), "%s",
                    m_edit->normal_material.specular_path.c_str());
      std::snprintf(m_matBurnedBuf, sizeof(m_matBurnedBuf), "%s",
                    m_edit->normal_material.diffuse_path.c_str());
    }
    if (sel)
      ImGui::SetItemDefaultFocus();
  }
}

void PrefabBrowserPanel::DrawHeader(const std::string &id) {
  ImGui::Text("Prefab: %s", id.c_str());
  ImGui::Separator();
}

void PrefabBrowserPanel::DrawType(AgentPrefab &P) {
  int t = (int)P.m_type;
  const char *types[] = {"Tree", "Burned", "Fire", "House"};
  if (ImGui::Combo("Type", &t, types, IM_ARRAYSIZE(types))) {
    P.m_type = static_cast<FireAgentState>(t);
  }
}

void PrefabBrowserPanel::DrawModelSection(AgentPrefab &P) {
  if (ImGui::InputText("Model path", m_modelBuf, IM_ARRAYSIZE(m_modelBuf))) {
    P.m_modelPath = m_modelBuf;
  }
  DragVec3("Model scale", P.m_modelScale, 0.01F);
}

void PrefabBrowserPanel::DrawBounds(AgentPrefab &P) {
  if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen)) {
    DragVec3("Min", P.m_bounds_min, 0.01f);
    DragVec3("Max", P.m_bounds_max, 0.01f);
  }
}

void PrefabBrowserPanel::DrawPhysics(AgentPrefab &P) {
  if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat("Intensity", &P.m_physics.m_intencity, 0.05f, 0.0f,
                     1000.0f);
  }
}


void PrefabBrowserPanel::DrawFireSection(AgentPrefab &P) {
  if (!ImGui::CollapsingHeader("Fire", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  // ID материала (строка из FireConfig, пока редактируем руками)
  if (ImGui::InputText("Material id", m_fireMaterialBuf,
                       IM_ARRAYSIZE(m_fireMaterialBuf))) {
    P.m_fireMaterialId = m_fireMaterialBuf;
  }

  // Мультипликатор температуры воспламенения
  ImGui::DragFloat("Ignition mult", &P.m_fireIgnitionMult,
                   0.01f, 0.1f, 10.0f);

  // Смещение влажности (от -1 до 1)
  ImGui::DragFloat("Moisture offset", &P.m_fireMoistureOffset,
                   0.01f, -1.0f, 1.0f);
  // Множитель топлива
  ImGui::DragFloat("Fuel mult", &P.m_fireFuelMult,
                 0.01f, 0.5f, 2.0f);
}


void PrefabBrowserPanel::DrawFirePoints(AgentPrefab &P) {
  if (!ImGui::CollapsingHeader("Fire points", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  if (ImGui::Button("Add point")) {
    P.m_firePoints.push_back({glm::vec3(0), 100.0f});
  }
  for (int i = 0; i < (int)P.m_firePoints.size(); ++i) {
    ImGui::PushID(i);
    ImGui::Separator();
    DragVec3("Pos", P.m_firePoints[i].m_position, 0.01f);
    ImGui::DragFloat("Fuel", &P.m_firePoints[i].m_initialFuel, 0.5f, 0.0f,
                     10000.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove")) {
      P.m_firePoints.erase(P.m_firePoints.begin() + i);
      ImGui::PopID();
      break;
    }
    ImGui::PopID();
  }
}

void PrefabBrowserPanel::DrawVisuals(AgentPrefab &P) {
  if (!ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  bool changed = false;

  changed |=
      ImGui::InputText("Normal", m_matNormalBuf, IM_ARRAYSIZE(m_matNormalBuf));
  changed |= ImGui::InputText("Burning", m_matBurningBuf,
                              IM_ARRAYSIZE(m_matBurningBuf));
  changed |=
      ImGui::InputText("Burned", m_matBurnedBuf, IM_ARRAYSIZE(m_matBurnedBuf));

  // Цвета
  changed |=
      ImGui::ColorEdit3("Normal diffuse", &P.normal_material.diffuse_color[0]);
  changed |= ImGui::ColorEdit3("Normal specular",
                               &P.normal_material.specular_color[0]);
  changed |= ImGui::ColorEdit3("Burning diffuse",
                               &P.burning_material.diffuse_color[0]);
  changed |= ImGui::ColorEdit3("Burning specular",
                               &P.burning_material.specular_color[0]);
  changed |=
      ImGui::ColorEdit3("Burned diffuse", &P.burned_material.diffuse_color[0]);
  changed |= ImGui::ColorEdit3("Burned specular",
                               &P.burned_material.specular_color[0]);

  // Шайнинесс
  changed |= ImGui::DragFloat("Normal shininess", &P.normal_material.shininess,
                              1.0f, 0.0f, 256.0f);
  changed |= ImGui::DragFloat(
      "Burning shininess", &P.burning_material.shininess, 1.0f, 0.0f, 256.0f);
  changed |= ImGui::DragFloat("Burned shininess", &P.burned_material.shininess,
                              1.0f, 0.0f, 256.0f);

  if (changed) {
    P.normal_material.diffuse_path = m_matNormalBuf;
    P.normal_material.specular_path = m_matNormalBuf;
    P.normal_material.normal_path = m_matNormalBuf;

    P.burning_material.diffuse_path = m_matBurningBuf;
    P.burning_material.specular_path = m_matBurningBuf;
    P.burning_material.normal_path = m_matBurningBuf;

    P.burned_material.diffuse_path = m_matBurnedBuf;
    P.burned_material.specular_path = m_matBurnedBuf;
    P.burned_material.normal_path = m_matBurnedBuf;
  }
}

void PrefabBrowserPanel::DrawSaveButtons(EditorContext &ctx,
                                         const std::string &id,
                                         AgentPrefab &P) {
  static float tConfirm = -1000.f;

  if (ImGui::Button("Apply (runtime)")) {
    ctx.m_scene->GetPrefabSystem().SavePrefab(id, P);
    P.LoadMaterials();
    ctx.m_scene->ReloadModelForPrefab(id, P);
    tConfirm = (float)ImGui::GetTime();
  }
  if ((float)ImGui::GetTime() - tConfirm < 1.0f) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved");
  }

  ImGui::SameLine();

  if (ImGui::Button("Save to file")) {
    if (m_savePath.empty())
      m_savePath = "res/prefabs/" + id + ".yaml";
    std::snprintf(m_saveBuf, sizeof(m_saveBuf), "%s", m_savePath.c_str());
    m_savePopupOpen = true;
    ImGui::OpenPopup("Save Prefab");
    P.LoadMaterials();
    ctx.m_scene->ReloadModelForPrefab(id, P);
  }
}

void PrefabBrowserPanel::DrawSavePopup(EditorContext &ctx,
                                       const std::string &id, AgentPrefab &P) {
  if (!ImGui::BeginPopupModal("Save Prefab", &m_savePopupOpen,
                              ImGuiWindowFlags_AlwaysAutoResize))
    return;

  ImGui::InputText("Path", m_saveBuf, IM_ARRAYSIZE(m_saveBuf));

  if (ImGui::Button("Save")) {
    m_savePath = m_saveBuf;

    auto &sys = ctx.m_scene->GetPrefabSystem();

    std::string newId;
    {
      std::filesystem::path out(m_savePath);
      newId = out.stem().string();
      if (newId.empty())
        newId = id; // fallback
    }
    sys.SavePrefab(newId, P);

    sys.SavePrefabToFile(newId, m_savePath);

    m_selectedIndex = 0;
    const auto &ids = ctx.m_scene->ListPrefabIds();
    for (int i = 0; i < (int)ids.size(); ++i) {
      if (ids[i] == newId) {
        m_selectedIndex = i;
        break;
      }
    }
    m_edit.reset();
    ImGui::CloseCurrentPopup();
    m_savePopupOpen = false;
  }

  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    ImGui::CloseCurrentPopup();
    m_savePopupOpen = false;
  }
  ImGui::EndPopup();
}

} // namespace ui
