// src/editor/EditorExtension.h
#pragma once
#include "app/IAppExtension.h"
#include "ui/Editor.h" 
#include "ui/EditorContext.h"
#include "ui/backends/ImGuiBackend.h" 
#include <memory>

class EditorExtension final : public IAppExtension {
public:

  void OnInit(engine::Application& app) override;
  void OnBeginUI(engine::Application& app, ui::EditorContext& ctx) override; 
  void OnEndUI(engine::Application& app, ui::EditorContext& ctx) override;   
  void OnShutdown(engine::Application& app) override;
  ~EditorExtension();

private:
  std::unique_ptr<ui::ImGuiBackend> m_imgui;
  std::unique_ptr<ui::Editor> m_editor;
};
