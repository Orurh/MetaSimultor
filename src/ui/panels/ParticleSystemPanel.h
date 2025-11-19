#pragma once
#include "graphics/ParticleSystem/ParticleSystem.h"
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
  void SaveToFile(const std::string &path);
  void LoadFromFile(const std::string &path);
  YAML::Node ToYAML() const;
  void FromYAML(const YAML::Node &node);

  std::shared_ptr<engine::ParticleSystem> m_selectedSystem;
  std::shared_ptr<engine::ParticleEffect> m_currentEffect;

  // Для работы с ассетами систем частиц
  std::shared_ptr<engine::ParticleEffect> m_editingEffect;
  std::string m_selectedAsset;
};

}  // namespace ui