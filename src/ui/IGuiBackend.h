// ui/IGuiBackend.h
#pragma once
#include "ui/Interface.h"
struct GLFWwindow;
namespace ui {
class IGuiBackend : public Interface {
public:
  virtual void Init(GLFWwindow* win, const char* glsl) = 0;
  virtual void Shutdown() = 0;
  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;
  virtual void DrawDockspace() = 0;

  // NEW: раскладки
  virtual void SaveLayout(const char* path) { /* optional */ }
  virtual void LoadLayout(const char* path) { /* optional */ }
  virtual void UpdateUIScale(GLFWwindow* window) { /* optional */ }
};
} // namespace ui
