// modules/BasicToolsModule.cc
#include "ui/modules/BasicToolsModule.h"
#include "ui/Editor.h"
#include "ui/panels/FireParamsPanel.h"
#include "ui/panels/FoliageBrushTool.h"
#include "ui/panels/ParticleSystemPanel.h"
#include "ui/panels/PrefabBrowserPanel.h"
#include "ui/panels/SceneIOPanel.h"
#include "ui/panels/TerrainConfigPanel.h"
#include "ui/panels/ToolbarPanel.h"
#include "ui/panels/WindPanel.h"
#include "ui/tools/PlaceOnGroundTool.h"
namespace ui {

void BasicToolsModule::Startup(Editor &editor) {
  editor.RegisterPanel(std::make_unique<ToolbarPanel>());
  editor.RegisterTool(std::make_unique<PlaceOnGroundTool>(
      "res/models/LowPolytree/CartoonTree.obj"));
  editor.RegisterPanel(std::make_unique<ui::SceneIOPanel>());
  editor.RegisterPanel(std::make_unique<ui::WindPanel>());
  editor.RegisterPanel(std::make_unique<ui::PrefabBrowserPanel>());
  editor.RegisterPanel(std::make_unique<ui::TerrainConfigPanel>());
  editor.RegisterTool(std::make_unique<FoliageBrushTool>(""));
  editor.RegisterPanel(std::make_unique<ParticleSystemPanel>());
}

void BasicToolsModule::Shutdown() {
  // Если у модуля есть внешние ресурсы — освободить здесь.
  // Удалять панели/инструменты из Editor не обязательно:
  // они принадлежат Editor и разрушатся вместе с ним.
}

} // namespace ui
