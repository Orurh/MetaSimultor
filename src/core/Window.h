// clang-format off
// NOLINTBEGIN(llvm-include-order)
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// NOLINTEND(llvm-include-order)
// clang-format on

#include "Input.h"
#include <string>

namespace engine {
class Window {
  static void MousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win != nullptr) {
      InputSystem::Instance().SetMousePosition(xpos, ypos);
    }
  }

  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win != nullptr) {
      InputSystem::Instance().SetMouseButtonState(button, action == GLFW_PRESS);
    }
  }
  static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win != nullptr) {
      InputSystem::Instance().SetMouseScrollY(yoffset);
    }
  }

 public:
  Window(int width, int height, const std::string& title);
  ~Window();
  void PollEvents();
  [[nodiscard]] bool ShouldClose() const;
  void SwapBuffers();
  [[nodiscard]] GLFWwindow* GetWindow() const;

 private:
  GLFWwindow* m_window;
};
}  // namespace engine