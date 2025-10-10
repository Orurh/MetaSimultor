#include "Window.h"
#include "Input.h"

#include <stdexcept>
namespace engine {
static std::unordered_map<int, engine::KeyCode> keyMap = {
    {GLFW_KEY_W, engine::KeyCode::W},           {GLFW_KEY_A, engine::KeyCode::A},
    {GLFW_KEY_S, engine::KeyCode::S},           {GLFW_KEY_D, engine::KeyCode::D},
    {GLFW_KEY_SPACE, engine::KeyCode::Space},   {GLFW_KEY_ENTER, engine::KeyCode::Enter},
    {GLFW_KEY_ESCAPE, engine::KeyCode::Escape}, {GLFW_KEY_UP, engine::KeyCode::Up},
    {GLFW_KEY_DOWN, engine::KeyCode::Down},     {GLFW_KEY_LEFT, engine::KeyCode::Left},
    {GLFW_KEY_RIGHT, engine::KeyCode::Right}};
Window::Window(int width, int height, const std::string& title) {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to start GLFW");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  m_window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  if (!m_window_) {
    glfwTerminate();
    throw std::runtime_error("Failed to initialize window");
  }
  glfwMakeContextCurrent(m_window_);
  glfwSetCursorPosCallback(m_window_, MousePositionCallback);
  glfwSetMouseButtonCallback(m_window_, MouseButtonCallback);
  glfwSetWindowUserPointer(m_window_, this);
  glfwSetScrollCallback(m_window_, MouseScrollCallback);
}
Window::~Window() {
  if (m_window_) {
    glfwDestroyWindow(m_window_);
  }
  glfwTerminate();
}
void Window::PollEvents() {
  glfwPollEvents();

  for (const auto& [glfwKey, ourKey] : keyMap) {
    bool pressed = glfwGetKey(m_window_, glfwKey) == GLFW_PRESS;

    InputSystem::Instance().SetKeyState(ourKey, pressed);
  }

  InputSystem::Instance().Update();
}
bool Window::ShouldClose() const {
  return glfwWindowShouldClose(m_window_) != 0;
}
void Window::SwapBuffers() {
  glfwSwapBuffers(m_window_);
}
GLFWwindow* Window::GetWindow() const {
  return m_window_;
}
}  // namespace engine
