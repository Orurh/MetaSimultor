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

void ToolbarPanel::Draw(EditorContext& ctx) {
  auto& st = ctx.State();

  // режим
  bool isPrefab = (st.m_placeKind == EditorState::PlaceKind::PREFAB);
  if (ImGui::RadioButton("Prefab##mode", isPrefab)) {
    st.m_placeKind = EditorState::PlaceKind::PREFAB;
}
  ImGui::SameLine();
  bool isFire = (st.m_placeKind == EditorState::PlaceKind::FIRE);
  if (ImGui::RadioButton("Fire##mode", isFire)) {
    st.m_placeKind = EditorState::PlaceKind::FIRE;
}

  // огонь
  if (st.m_placeKind == EditorState::PlaceKind::FIRE) {
    ImGui::SliderFloat("Fire intensity", &st.m_fireIntensity, 0.f, 10.f);
    ctx.m_scene->SetFireIntensity(st.m_fireIntensity);
    // … кнопки поджига как у тебя, только в AddFire передавать st.fireIntensity
  }

  // префабы
  if (st.m_placeKind == EditorState::PlaceKind::PREFAB) {
    const auto& ids = ctx.m_scene->ListPrefabIds();
    if (!ids.empty()) {
      if (std::ranges::find(ids, st.m_selectedPrefab) == ids.end()) {
        st.m_selectedPrefab = ids.front();
}
      if (ImGui::BeginCombo("Prefab##combo", st.m_selectedPrefab.c_str())) {
        for (const auto& id : ids) {
          bool sel = (id == st.m_selectedPrefab);
          if (ImGui::Selectable(id.c_str(), sel)) st.m_selectedPrefab = id;
          if (sel) { ImGui::SetItemDefaultFocus();
}
        }
        ImGui::EndCombo();
      }
    } else {
      ImGui::TextDisabled("no prefabs loaded");
    }
  }
}




}  // namespace ui
