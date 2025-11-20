// ui/panels/ToolbarPanel.cpp
#include "ui/panels/ToolbarPanel.h"
#include "ui/panels/FoliageBrushTool.h"
#include "ui/tools/PlaceOnGroundTool.h"

#include <algorithm>
#include <iostream>
#include "core/Input.h"  // ← у тебя именно так называется инпут
#include "core/scene/Scene.h"
#include "imgui.h"
#include "math/ScreenRay.h"  // луч из экрана
#include "ui/Editor.h"

namespace ui {

void ToolbarPanel::Draw(EditorContext& ctx) {
  // 0) переключатель воркспейса
  DrawWorkspaceSwitch(ctx);

  ImGui::Separator();

  // 1) режимы (Prefab / Fire / Ignite)
  DrawModeSwitch(ctx);

  ImGui::Separator();

  // 2) селектор инструментов
  DrawToolSelector(ctx);

  ImGui::Separator();

  // 3) UI активного инструмента
  DrawActiveToolUI(ctx);
}

void ToolbarPanel::DrawWorkspaceSwitch(EditorContext& ctx) {
  auto& st = ctx.State();
  using W = EditorState::Workspace;

  int current = 0;
  switch (st.m_workspace) {
    case W::ALL:    current = 0; break;
    case W::SCENE:  current = 1; break;
    case W::PREFAB: current = 2; break;
    case W::FIRE:   current = 3; break;
    case W::FX:     current = 4; break;
    case W::TERRAIN:current = 5; break;
  }

  const char* labels[] = {
    "All",
    "Scene",
    "Prefabs",
    "Fire",
    "FX",
    "Terrain",
  };

  if (ImGui::Combo("Workspace", &current, labels, IM_ARRAYSIZE(labels))) {
    switch (current) {
      case 0: st.m_workspace = W::ALL;    break;
      case 1: st.m_workspace = W::SCENE;  break;
      case 2: st.m_workspace = W::PREFAB; break;
      case 3: st.m_workspace = W::FIRE;   break;
      case 4: st.m_workspace = W::FX;     break;
      case 5: st.m_workspace = W::TERRAIN;break;
    }
  }
}



void ToolbarPanel::DrawModeSwitch(EditorContext& ctx) {
  auto& st = ctx.State();

  bool isPrefab = (st.m_placeKind == EditorState::PlaceKind::PREFAB);
  if (ImGui::RadioButton("Prefab##mode", isPrefab)) {
    st.m_placeKind = EditorState::PlaceKind::PREFAB;
  }
  ImGui::SameLine();

  bool isFire = (st.m_placeKind == EditorState::PlaceKind::FIRE);
  if (ImGui::RadioButton("Fire##mode", isFire)) {
    st.m_placeKind = EditorState::PlaceKind::FIRE;
    if (ctx.m_editor != nullptr) {
      ctx.m_editor->SetActiveTool(ui::ToolType::PLACE_ON_GROUND, ctx);
    }
  }
  ImGui::SameLine();
    bool isIgnite = (st.m_placeKind == EditorState::PlaceKind::IGNITE);
  if (ImGui::RadioButton("Ignite##mode", isIgnite)) {
    st.m_placeKind = EditorState::PlaceKind::IGNITE;
    if (ctx.m_editor != nullptr) {
      ctx.m_editor->SetActiveTool(ui::ToolType::PLACE_ON_GROUND, ctx);
    }
  }

  // огонь — его параметры сразу тут
  if (st.m_placeKind == EditorState::PlaceKind::FIRE) {
    ImGui::SliderFloat("Fire intensity", &st.m_fireIntensity, 0.f, 10.f);
    if (ctx.m_scene)
      ctx.m_scene->SetFireIntensity(st.m_fireIntensity);
    // при необходимости: кнопки запуска/остановки/сброса огня
  }

  // префабы — выбор актуального префаба
  if (st.m_placeKind == EditorState::PlaceKind::PREFAB) {
    if (ctx.m_scene) {
      const auto& ids = ctx.m_scene->ListPrefabIds();
      if (!ids.empty()) {
        if (std::ranges::find(ids, st.m_selectedPrefab) == ids.end()) {
          st.m_selectedPrefab = ids.front();
        }
        if (ImGui::BeginCombo("Prefab##combo", st.m_selectedPrefab.c_str())) {
          for (const auto& id : ids) {
            bool sel = (id == st.m_selectedPrefab);
            if (ImGui::Selectable(id.c_str(), sel))
              st.m_selectedPrefab = id;
            if (sel)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
      } else {
        ImGui::TextDisabled("no prefabs loaded");
      }
    }
  }
}

void ToolbarPanel::DrawToolSelector(EditorContext& ctx) {
  // тут просто переключаем активный тул
  const auto active = ctx.GetActiveToolType();

  if (ImGui::Selectable("Place on ground", active == ToolType::PLACE_ON_GROUND)) {
    if (ctx.m_editor)
      ctx.m_editor->SetActiveTool(ToolType::PLACE_ON_GROUND, ctx);
  }

  if (ImGui::Selectable("Foliage Brush", active == ToolType::OTHER_TOOL)) {  // пока OTHER_TOOL
    if (ctx.m_editor)
      ctx.m_editor->SetActiveTool(ToolType::OTHER_TOOL, ctx);
  }
}

// ui/panels/ToolbarPanel.cpp

void ToolbarPanel::DrawActiveToolUI(EditorContext& ctx) {
  const auto active = ctx.GetActiveToolType();
  constexpr ImGuiWindowFlags kWinFlags =
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

  switch (active) {
    case ToolType::PLACE_ON_GROUND: {
      bool open = true;  // без крестика можно передать nullptr
      if (ImGui::Begin("Place on ground", &open, kWinFlags)) {
        if (auto* t = ctx.m_editor ? ctx.m_editor->GetTool<ui::PlaceOnGroundTool>() : nullptr) {
          // если нет контролов — можно текст “no settings”
          // t->DrawControls(ctx);
          ImGui::TextDisabled("No settings.");
        }
      }
      ImGui::End();
      break;
    }
    case ToolType::OTHER_TOOL: {  // Foliage Brush
      bool open = true;
      if (ImGui::Begin("Foliage Brush", &open, kWinFlags)) {
        if (auto* t = ctx.m_editor ? ctx.m_editor->GetTool<FoliageBrushTool>() : nullptr) {
          t->DrawControls(ctx);  // ВАЖНО: без Begin/End внутри
        }
      }
      ImGui::End();
      break;
    }
    default:
      break;
  }
}

}  // namespace ui
