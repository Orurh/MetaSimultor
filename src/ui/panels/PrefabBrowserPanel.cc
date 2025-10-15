#include "ui/panels/PrefabBrowserPanel.h"
#include "ui/EditorContext.h"
#include "core/scene/Scene.h"
#include "data/PrefabSystem.h"
#include <imgui.h>
#include <filesystem>

using engine::data::AgentPrefab;

namespace ui {

    inline bool DragVec3(const char* label,
                     glm::vec3& v,
                     float speed = 0.1F,
                     float v_min = -FLT_MAX,
                     float v_max =  FLT_MAX,
                     const char* fmt = "%.3f")
{
    float a[3] = { v.x, v.y, v.z };
    const bool CHANGED = ImGui::DragFloat3(label, a, speed, v_min, v_max, fmt);
    if (CHANGED) { v.x = a[0]; v.y = a[1]; v.z = a[2]; }
    return CHANGED;
}


void PrefabBrowserPanel::Draw(EditorContext& ctx) {
  bool open = true;
  if (!ImGui::Begin(Id(), &open)) { ImGui::End(); return; }

  const auto& ids = ctx.m_scene->ListPrefabIds();
  if (ids.empty()) {
    ImGui::TextDisabled("No prefabs loaded");
    ImGui::End();
    return;
  }

  // Поддерживаем корректный выбор и m_edit
  const std::string& id = EnsureSelection(ctx, ids);

  // ===== ЛЕВО =====
  ImGui::BeginChild("##left", ImVec2(220, 0), true);
  DrawLeftColumn(ctx, ids);
  ImGui::EndChild();
  ImGui::SameLine();

  // ===== ПРАВО =====
  ImGui::BeginChild("##right", ImVec2(0, 0), true);
  AgentPrefab& P = *m_edit;
  DrawHeader(id);

  DrawType(P);
  DrawModelSection(P);
  DrawBounds(P);
  DrawPhysics(P);
  DrawFirePoints(P);
  DrawVisuals(P);

  ImGui::Separator();
  DrawSaveButtons(ctx, id, P);
  DrawSavePopup(ctx, id, P);
  ImGui::EndChild();

  ImGui::End();
}

// ------------------ helpers ------------------

const std::string& PrefabBrowserPanel::EnsureSelection(EditorContext& ctx, const std::vector<std::string>& ids) {
  if (m_selectedIndex < 0 || m_selectedIndex >= (int)ids.size()) {
    m_selectedIndex = 0;
    m_edit.reset();
  }
  const std::string& id = ids[m_selectedIndex];
  if (!m_edit.has_value()) {
    m_edit = ctx.m_scene->GetPrefabSystem().GetPrefab(id);
    // Синхронизируем текстовые буферы один раз при загрузке
    std::snprintf(m_modelBuf, sizeof(m_modelBuf), "%s", m_edit->m_modelPath.c_str());
    std::snprintf(m_matNormalBuf,  sizeof(m_matNormalBuf),  "%s", m_edit->normal_material.normal_path.c_str());
    std::snprintf(m_matBurningBuf, sizeof(m_matBurningBuf), "%s", m_edit->normal_material.specular_path.c_str());
    std::snprintf(m_matBurnedBuf,  sizeof(m_matBurnedBuf),  "%s", m_edit->normal_material.diffuse_path.c_str());
  }
  return id;
}

void PrefabBrowserPanel::DrawLeftColumn(EditorContext& ctx, const std::vector<std::string>& ids) {
  for (int i = 0; i < (int)ids.size(); ++i) {
    const bool sel = (i == m_selectedIndex);
    if (ImGui::Selectable(ids[i].c_str(), sel)) {
      m_selectedIndex = i;
      // Перезагружаем редактируемую копию и буферы
      m_edit = ctx.m_scene->GetPrefabSystem().GetPrefab(ids[i]);
      std::snprintf(m_modelBuf, sizeof(m_modelBuf), "%s", m_edit->m_modelPath.c_str());
      std::snprintf(m_matNormalBuf,  sizeof(m_matNormalBuf),  "%s", m_edit->normal_material.normal_path.c_str());
      std::snprintf(m_matBurningBuf, sizeof(m_matBurningBuf), "%s", m_edit->normal_material.specular_path.c_str());
      std::snprintf(m_matBurnedBuf,  sizeof(m_matBurnedBuf),  "%s", m_edit->normal_material.diffuse_path.c_str());
    }
    if (sel) ImGui::SetItemDefaultFocus();
  }
}

void PrefabBrowserPanel::DrawHeader(const std::string& id) {
  ImGui::Text("Prefab: %s", id.c_str());
  ImGui::Separator();
}

void PrefabBrowserPanel::DrawType(AgentPrefab& P) {
  int t = (int)P.m_type;
  const char* types[] = { "Tree", "Burned", "Fire", "House" };
  if (ImGui::Combo("Type", &t, types, IM_ARRAYSIZE(types))) {
    P.m_type = static_cast<AgentType>(t);
  }
}

void PrefabBrowserPanel::DrawModelSection(AgentPrefab& P) {
  if (ImGui::InputText("Model path", m_modelBuf, IM_ARRAYSIZE(m_modelBuf))) {
    P.m_modelPath = m_modelBuf;
  }
  DragVec3("Model scale", P.m_modelScale, 0.01F);
}

void PrefabBrowserPanel::DrawBounds(AgentPrefab& P) {
  if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen)) {
    DragVec3("Min", P.m_bounds_min, 0.01f);
    DragVec3("Max", P.m_bounds_max, 0.01f);
  }
}

void PrefabBrowserPanel::DrawPhysics(AgentPrefab& P) {
  if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat("Intensity", &P.m_physics.m_intencity, 0.05f, 0.0f, 1000.0f);
  }
}

void PrefabBrowserPanel::DrawFirePoints(AgentPrefab& P) {
  if (!ImGui::CollapsingHeader("Fire points", ImGuiTreeNodeFlags_DefaultOpen)) return;

  if (ImGui::Button("Add point")) {
    P.m_firePoints.push_back({glm::vec3(0), 100.0f});
  }
  for (int i = 0; i < (int)P.m_firePoints.size(); ++i) {
    ImGui::PushID(i);
    ImGui::Separator();
    DragVec3("Pos", P.m_firePoints[i].m_position, 0.01f);
    ImGui::DragFloat("Fuel", &P.m_firePoints[i].m_initialFuel, 0.5f, 0.0f, 10000.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove")) {
      P.m_firePoints.erase(P.m_firePoints.begin() + i);
      ImGui::PopID();
      break; // корректный выход из цикла после erase
    }
    ImGui::PopID();
  }
}

void PrefabBrowserPanel::DrawVisuals(AgentPrefab& P) {
  if (!ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen)) return;

  bool changed = false;
  changed |= ImGui::InputText("Normal",  m_matNormalBuf,  IM_ARRAYSIZE(m_matNormalBuf));
  changed |= ImGui::InputText("Burning", m_matBurningBuf, IM_ARRAYSIZE(m_matBurningBuf));
  changed |= ImGui::InputText("Burned",  m_matBurnedBuf,  IM_ARRAYSIZE(m_matBurnedBuf));
  if (changed) {
    P.normal_material.normal_path = m_matNormalBuf;
    P.normal_material.diffuse_path = m_matNormalBuf;
    P.normal_material.specular_path = m_matNormalBuf;
    P.burning_material.normal_path = m_matBurningBuf;
    P.burning_material.diffuse_path = m_matBurningBuf;
    P.burning_material.specular_path = m_matBurningBuf;
    P.burned_material.normal_path = m_matBurnedBuf;
    P.burned_material.diffuse_path = m_matBurnedBuf;
    P.burned_material.specular_path = m_matBurnedBuf;
  }
}

void PrefabBrowserPanel::DrawSaveButtons(EditorContext& ctx, const std::string& id, AgentPrefab& P) {
  static float tConfirm = -1000.f;

  if (ImGui::Button("Apply (runtime)")) {
    // Обновляем в системе (в памяти)
    ctx.m_scene->GetPrefabSystem().SavePrefab(id, P); // интерфейс PrefabSystem
    tConfirm = (float)ImGui::GetTime();
  }
  if ((float)ImGui::GetTime() - tConfirm < 1.0f) {
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "Saved");
  }

  ImGui::SameLine();

  if (ImGui::Button("Save to file")) {
    if (m_savePath.empty()) m_savePath = "res/prefabs/" + id + ".yaml";
    std::snprintf(m_saveBuf, sizeof(m_saveBuf), "%s", m_savePath.c_str());
    m_savePopupOpen = true;
    ImGui::OpenPopup("Save Prefab");
  }
}

void PrefabBrowserPanel::DrawSavePopup(EditorContext& ctx, const std::string& id, AgentPrefab& P) {
  if (!ImGui::BeginPopupModal("Save Prefab", &m_savePopupOpen, ImGuiWindowFlags_AlwaysAutoResize))
    return;

  ImGui::InputText("Path", m_saveBuf, IM_ARRAYSIZE(m_saveBuf));

  if (ImGui::Button("Save")) {
    m_savePath = m_saveBuf;

    auto& sys = ctx.m_scene->GetPrefabSystem();

    // --- новый id берём из имени файла (res/prefabs/<newId>.yaml) ---
    // если хочешь иначе — поменяй стратегию (читать из YAML, хранить поле id и т.д.)
    std::string newId;
    {
      // <filesystem> должен быть подключён где-то один раз в TU
      // #include <filesystem>
      std::filesystem::path out(m_savePath);
      newId = out.stem().string();
      if (newId.empty()) newId = id; // fallback
    }

    // 1) гарантируем актуальность рантайма под НОВЫМ id
    // В большинстве реализаций SavePrefab(id, P) добавляет/заменяет запись в реестре.
    // Если у тебя отдельный метод AddOrReplacePrefab, зови его здесь вместо SavePrefab.
    sys.SavePrefab(newId, P);

    // 2) пишем на диск под НОВЫМ id
    sys.SavePrefabToFile(newId, m_savePath);

    // 3) обновляем список префабов в системе (перескан каталога)
    // Если в твоём PrefabSystem нет ReloadAll(), замени это место на ImportFromFile(m_savePath)
    // или просто оставь шаг 1) — он уже внёс запись в реестр.
    // sys.ReloadAll();

    // 4) перевыбор элемента в левом списке и сброс локального кэша
    m_selectedIndex = 0;
    const auto& ids = ctx.m_scene->ListPrefabIds();
    for (int i = 0; i < (int)ids.size(); ++i) {
      if (ids[i] == newId) { m_selectedIndex = i; break; }
    }
    m_edit.reset(); // EnsureSelection() подхватит свежую копию и синхронизирует буферы

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
