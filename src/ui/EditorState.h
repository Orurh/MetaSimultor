// ui/EditorState.h
#pragma once
#include <cstdint>
#include <string>

namespace ui {
struct EditorState {

  enum class Workspace : uint8_t { SCENE, PREFAB, TERRAIN, FIRE, FX, ALL };
  Workspace m_workspace = Workspace::SCENE;
  // какой префаб сейчас выбран в браузере префабов
  enum class PlaceKind : std::uint8_t { PREFAB, FIRE, IGNITE };
  PlaceKind m_placeKind = PlaceKind::PREFAB;
  std::string m_selectedPrefab = "default_tree";
  float m_fireIntensity = 1.0F;
};
}  // namespace ui
