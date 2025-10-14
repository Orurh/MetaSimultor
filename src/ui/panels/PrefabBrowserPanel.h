// ui/panels/PrefabBrowserPanel.h
#pragma once
#include "data/PrefabSystem.h"
#include "ui/IPanel.h"
#include <optional>
#include <string>
#include <vector>

namespace ui {

class PrefabBrowserPanel final : public IPanel {
public:
  [[nodiscard]] const char* Id() const override { return "Prefabs"; }
  void Draw(EditorContext& ctx) override;

private:
  // разбиение
  const std::string& EnsureSelection(EditorContext& ctx, const std::vector<std::string>& ids);
  void DrawLeftColumn(EditorContext& ctx, const std::vector<std::string>& ids);
  void DrawHeader(const std::string& id);
  void DrawType(engine::data::AgentPrefab& P);
  void DrawModelSection(engine::data::AgentPrefab& P);
  void DrawBounds(engine::data::AgentPrefab& P);
  void DrawPhysics(engine::data::AgentPrefab& P);
  void DrawFirePoints(engine::data::AgentPrefab& P);
  void DrawVisuals(engine::data::AgentPrefab& P);
  void DrawSaveButtons(EditorContext& ctx, const std::string& id, engine::data::AgentPrefab& P);
  void DrawSavePopup(EditorContext& ctx, const std::string& id, engine::data::AgentPrefab& P);


  int m_selectedIndex = -1;
  std::optional<engine::data::AgentPrefab> m_edit;

  // постоянные текстовые буферы (чтобы не форматировать каждый кадр)
  char m_modelBuf[512]{};
  char m_matNormalBuf[256]{};
  char m_matBurningBuf[256]{};
  char m_matBurnedBuf[256]{};

  // сохранение в файл
  bool m_savePopupOpen = false;
  std::string m_savePath;
  char m_saveBuf[512]{};
};

} // namespace ui
