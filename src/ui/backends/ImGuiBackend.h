// ui/backends/ImGuiBackend.h
#pragma once
#include "ui/IGuiBackend.h"
namespace ui {
class ImGuiBackend final : public IGuiBackend {
public:
  void Init(GLFWwindow* ui_window, const char* glsl) override;
  void Shutdown() override;
  void BeginFrame() override;
  void EndFrame() override;
  void DrawDockspace() override;

  // NEW
  void SaveLayout(const char* path) override;
  void LoadLayout(const char* path) override;
  void UpdateUIScale(GLFWwindow* window) override;
};
} // namespace ui
