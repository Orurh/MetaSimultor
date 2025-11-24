#pragma once
#include "core/scene/heatMap.h"
#include "ui/IPanel.h"
#include <memory>
namespace engine {
class Texture;

}  // namespace engine

namespace ui {
class HeatMapPanel final : public IPanel {
  public:
  [[nodiscard]] const char *Id() const override { return "Heat Map"; }
  void Draw(EditorContext &ctx) override;

  private:
  std::shared_ptr<engine::Texture> m_texture;
  engine::HeatMap::Stats m_stats;
};
}  // namespace ui
