#pragma once
struct GLFWwindow;

namespace ui {
class ImGuiLayer {
 public:
  void Init(GLFWwindow* window, const char* glsl = "#version 330");
  void Shutdown();
  void BeginFrame();
  void EndFrame();
  void DrawDockspace();
};
}  // namespace ui