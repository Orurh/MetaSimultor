// ui/modules/PrefabModule.cc
#include "PrefabModule.h"
#include "ui/Editor.h"
#include "ui/panels/PrefabBrowserPanel.h"
#include "ui/panels/FoliageBrushTool.h"
#include "ui/tools/PlaceOnGroundTool.h"

namespace ui {

void PrefabModule::Startup(Editor& editor) {
  editor.RegisterPanel(std::make_unique<PrefabBrowserPanel>());
  editor.RegisterTool(std::make_unique<PlaceOnGroundTool>("res/models/LowPolytree/CartoonTree.obj"));
  editor.RegisterTool(std::make_unique<FoliageBrushTool>(""));
}

}  // namespace ui
