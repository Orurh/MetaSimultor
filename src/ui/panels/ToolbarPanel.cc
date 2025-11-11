// ui/panels/ToolbarPanel.cpp
#include "ui/panels/ToolbarPanel.h"

#include <algorithm>
#include <iostream>
#include "core/Input.h"  // ← у тебя именно так называется инпут
#include "core/scene/Scene.h"
#include "imgui.h"
#include "math/ScreenRay.h"  // луч из экрана
#include "ui/Editor.h"

namespace ui {

// void ToolbarPanel::Draw(EditorContext& ctx) {
//   auto& st = ctx.State();

//   // режим
//   bool isPrefab = (st.m_placeKind == EditorState::PlaceKind::PREFAB);
//   if (ImGui::RadioButton("Prefab##mode", isPrefab)) {
//     st.m_placeKind = EditorState::PlaceKind::PREFAB;
// }
//   ImGui::SameLine();
//   bool isFire = (st.m_placeKind == EditorState::PlaceKind::FIRE);
//   if (ImGui::RadioButton("Fire##mode", isFire)) {
//     st.m_placeKind = EditorState::PlaceKind::FIRE;
// }

//   // огонь
//   if (st.m_placeKind == EditorState::PlaceKind::FIRE) {
//     ImGui::SliderFloat("Fire intensity", &st.m_fireIntensity, 0.f, 10.f);
//     ctx.m_scene->SetFireIntensity(st.m_fireIntensity);
//     // … кнопки поджига как у тебя, только в AddFire передавать st.fireIntensity
//   }

//   // префабы
//   if (st.m_placeKind == EditorState::PlaceKind::PREFAB) {
//     const auto& ids = ctx.m_scene->ListPrefabIds();
//     if (!ids.empty()) {
//       if (std::ranges::find(ids, st.m_selectedPrefab) == ids.end()) {
//         st.m_selectedPrefab = ids.front();
// }
//       if (ImGui::BeginCombo("Prefab##combo", st.m_selectedPrefab.c_str())) {
//         for (const auto& id : ids) {
//           bool sel = (id == st.m_selectedPrefab);
//           if (ImGui::Selectable(id.c_str(), sel)) st.m_selectedPrefab = id;
//           if (sel) { ImGui::SetItemDefaultFocus();
// }
//         }
//         ImGui::EndCombo();
//       }
//     } else {
//       ImGui::TextDisabled("no prefabs loaded");
//     }
//   }
// }


void ToolbarPanel::Draw(EditorContext& ctx) {
  // 1) режимы (Prefab / Fire)
  DrawModeSwitch(ctx);

  ImGui::Separator();

  // 2) селектор инструментов (PlaceOnGround / FoliageBrush)
  DrawToolSelector(ctx);

  ImGui::Separator();

  // 3) панель активного инструмента (например, настройки кисти)
  DrawActiveToolUI(ctx);
}

// ─────────────────────────────────────────────────────────────────────────────
// private

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
  }

  // огонь — его параметры сразу тут
  if (st.m_placeKind == EditorState::PlaceKind::FIRE) {
    ImGui::SliderFloat("Fire intensity", &st.m_fireIntensity, 0.f, 10.f);
    if (ctx.m_scene) ctx.m_scene->SetFireIntensity(st.m_fireIntensity);
    // при необходимости: кнопки запуска/остановки/сброса огня
  }

  // префабы — выбор актуального префаба
  if (st.m_placeKind == EditorState::PlaceKind::PREFAB) {
    if (ctx.m_scene) {
      const auto& ids = ctx.m_scene->ListPrefabIds();
      if (!ids.empty()) {
        if (std::find(ids.begin(), ids.end(), st.m_selectedPrefab) == ids.end()) {
          st.m_selectedPrefab = ids.front();
        }
        if (ImGui::BeginCombo("Prefab##combo", st.m_selectedPrefab.c_str())) {
          for (const auto& id : ids) {
            bool sel = (id == st.m_selectedPrefab);
            if (ImGui::Selectable(id.c_str(), sel)) st.m_selectedPrefab = id;
            if (sel) ImGui::SetItemDefaultFocus();
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
    if (ctx.m_editor) ctx.m_editor->SetActiveTool(ToolType::PLACE_ON_GROUND, ctx);
  }

  if (ImGui::Selectable("Foliage Brush", active == ToolType::OTHER_TOOL)) { // пока OTHER_TOOL
    if (ctx.m_editor) ctx.m_editor->SetActiveTool(ToolType::OTHER_TOOL, ctx);
  }
}

void ToolbarPanel::DrawActiveToolUI(EditorContext& ctx) {
  // если активна кисть — рисуем её настройки
  if (auto* brush = ctx.m_editor ? ctx.m_editor->GetTool<ui::FoliageBrushTool>() : nullptr) {
    brush->DrawUI(ctx);
    return;
  }

  // опционально: другие инструменты могут иметь свой UI
  // if (auto* place = ctx.m_editor->GetTool<ui::PlaceOnGroundTool>()) { ... }
}




}  // namespace ui
