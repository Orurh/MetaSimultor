// ui/panels/PrefabBrowserPanel.h
#pragma once
#include "PrefabPreviewRenderer.h"
#include "data/PrefabSystem.h"
#include "imgui.h"
#include "ui/EditorContext.h"
#include "ui/IPanel.h"
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <string>
#include <vector>
namespace ui {

class PrefabBrowserPanel final : public IPanel {
public:
  [[nodiscard]] const char *Id() const override { return "Prefabs"; }
  void Draw(EditorContext &ctx) override;
  void OnShow(EditorContext &ctx) override;
  void OnHide(EditorContext &ctx) override;
  PrefabBrowserPanel();
  ~PrefabBrowserPanel();

private:
  void UpdatePreviewCamera(const engine::data::AgentPrefab &P);
  const std::string &EnsureSelection(EditorContext &ctx,
                                     const std::vector<std::string> &ids);
  void DrawLeftColumn(EditorContext &ctx, const std::vector<std::string> &ids);
  void DrawHeader(const std::string &id);
  void DrawType(engine::data::AgentPrefab &P);
  void DrawModelSection(engine::data::AgentPrefab &P);
  void DrawBounds(engine::data::AgentPrefab &P);
  void DrawPhysics(engine::data::AgentPrefab &P);
  void DrawFirePoints(engine::data::AgentPrefab &P);
  void DrawVisuals(engine::data::AgentPrefab &P);
  void DrawSaveButtons(EditorContext &ctx, const std::string &id,
                       engine::data::AgentPrefab &P);
  void DrawSavePopup(EditorContext &ctx, const std::string &id,
                     engine::data::AgentPrefab &P);
  void DrawModelPreview(EditorContext &ctx);
  void RenderPreview(engine::data::AgentPrefab &P);
  int m_selectedIndex = -1;
  std::optional<engine::data::AgentPrefab> m_edit;

  char m_modelBuf[512]{};
  char m_matNormalBuf[256]{};
  char m_matBurningBuf[256]{};
  char m_matBurnedBuf[256]{};

  bool m_savePopupOpen = false;
  std::string m_savePath;
  char m_saveBuf[512]{};
  std::unique_ptr<PrefabPreviewRenderer> m_preview;

  float m_previewYaw = 0, m_previewPitch = 0;
  bool m_dragging = false;
  ImVec2 m_lastMouse;
};

} // namespace ui
