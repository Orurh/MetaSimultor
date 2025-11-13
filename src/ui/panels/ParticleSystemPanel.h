#pragma once
#include "core/scene/Scene.h"
#include "graphics/ParticleSystem.h"
#include "ui/EditorContext.h"
#include "ui/IPanel.h"
#include <imgui.h>
#include <memory>

namespace ui {

class ParticleSystemPanel final : public IPanel {
public:
  ParticleSystemPanel();
  void Draw(EditorContext &ctx) override;
  const char *Id() const override { return "Particle System"; }

private:
  void DrawParticleSystemList(EditorContext &ctx);
  void DrawParticleSystemControls(EditorContext &ctx);
  void DrawEffectEditor();
  void DrawAddParticleSystemMenu(EditorContext &ctx);

  std::shared_ptr<engine::ParticleSystem> m_selectedSystem;
  std::shared_ptr<engine::ParticleEffect> m_currentEffect;
};

} // namespace ui