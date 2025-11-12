#pragma once
#include "ui/IPanel.h"
#include <array>

namespace ui {

// Панель сохранения/загрузки сцены
class SceneIOPanel final : public IPanel {
public:
  [[nodiscard]] const char* Id() const override { return "Scene I/O"; }
  void Draw(EditorContext& ctx) override;

private:
  // буфер для пути (редактируется в UI)
  std::array<char, 512> m_path{ "data/scene.yml" }; // поменяй дефолт под себя
  bool m_lastOk = true;        // статус последней операции
  const char* m_lastOp = "";   // "Save" или "Load"
};

} // namespace ui
