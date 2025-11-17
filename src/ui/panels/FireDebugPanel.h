// ui/panels/FireDebugPanel.h
#pragma once
#include "ui/IPanel.h"
#include "ui/EditorContext.h"
#include "core/agent.h"
// #include "core/AgentManager.h"     
// #include <optional>

namespace ui {

class FireDebugPanel final : public IPanel {
public:
  [[nodiscard]] const char* Id() const override { return "Fire Debug"; }
  void Draw(EditorContext& ctx) override;

private:
  bool  m_hasSelection = false;
  int   m_selectedGpuIndex = -1;
  Agent m_agentSnapshot{};      // копия агента с GPU
  int   m_pointIndex = 0;       // индекс точки для детального просмотра

  void HandlePick(EditorContext& ctx);
  void DrawAgentInfo();

  bool m_visible = false;   
};

} // namespace ui
