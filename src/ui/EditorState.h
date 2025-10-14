// ui/EditorState.h
#pragma once
#include <string>

namespace ui {
struct EditorState {
  // какой префаб сейчас выбран в браузере префабов
  enum class PlaceKind { PREFAB, FIRE };
  PlaceKind m_placeKind = PlaceKind::PREFAB;
  std::string m_selectedPrefab = "default_tree";
  float m_fireIntensity = 1.0F;
};
}  // namespace ui
