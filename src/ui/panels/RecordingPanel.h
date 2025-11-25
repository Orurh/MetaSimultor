
#pragma once

#include "core/snapshot/SceneRecorder.h"
#include "ui/IPanel.h"
namespace ui {
class RecordingPanel final : public IPanel {
  public:
  [[nodiscard]] const char *Id() const override { return "Recording"; }

  void Draw(EditorContext &ctx) override;

  private:
  private:
  void DrawRecorderUI(engine::SceneRecorder &recorder);
};

}  // namespace ui