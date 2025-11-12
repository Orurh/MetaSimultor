// ui/EditorState.h
#pragma once
#include <cstdint>
#include <string>

namespace ui {
struct EditorState {
  // какой префаб сейчас выбран в браузере префабов
  enum class PlaceKind : std::uint8_t { PREFAB, FIRE };
  PlaceKind m_placeKind = PlaceKind::PREFAB;
  std::string m_selectedPrefab = "default_tree";
  float m_fireIntensity = 1.0F;

  bool  placeScaleRandom = true;
  float placeScaleMin = 1.0f;
  float placeScaleMax = 1.0f;

};
}  // namespace ui
