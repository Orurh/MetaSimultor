#pragma once
#include "configs/FireParams.h"
#include "ui/IPanel.h"

namespace ui {

class FireParamsPanel final : public IPanel {
public:
  [[nodiscard]] const char *Id() const override { return "Fire Parameters"; }
  void Draw(EditorContext &ctx) override;

  FireParams &GetParams() { return m_params; }
  const FireParams &GetParams() const { return m_params; }

private:
  FireParams m_params;
};

} // namespace ui