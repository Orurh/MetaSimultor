#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <unordered_map>
namespace engine {

enum class KeyCode {
  W,
  A,
  S,
  D,
  Space,
  Enter,
  Escape,
  Up,
  Down,
  Left,
  Right,
};

enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

enum class InputState { Pressed, Released, Held };

class InputSystem {
public:
  static InputSystem &Instance() {
    static InputSystem inputSystem;
    return inputSystem;
  }
  void Update();
  bool GetKey(KeyCode key, InputState state = InputState::Held) const;
  bool GetKeyDown(KeyCode key) const {
    return GetKey(key, InputState::Pressed);
  }
  bool GetKeyUp(KeyCode key) const { return GetKey(key, InputState::Released); }
  void SetKeyState(KeyCode key, bool pressed);

  glm::vec2 GetMousePosition() const { return m_mousePosition; }
  glm::vec2 GetMouseDelta()    const { return m_mouseDelta; }

  bool GetMouseButton(MouseButton button, InputState state = InputState::Held) const;
  void SetMouseButtonState(MouseButton button, bool pressed);

  void SetMousePosition(float x, float y);

  float GetMouseScrollY() const { return m_mouseScrollY; }
  void  SetMouseScrollY(float yoffset) { m_mouseScrollY = yoffset; }
  void  ResetMouseScroll() { m_mouseScrollY = 0.0f; }
  void SetUICapture(bool mouseCaptured, bool keyboardCaptured) {
    m_uiMouseCaptured = mouseCaptured;
    m_uiKbdCaptured   = keyboardCaptured;
  }
  bool UIMouseCaptured()    const { return m_uiMouseCaptured; }
  bool UIKeyboardCaptured() const { return m_uiKbdCaptured; }

private:
  std::unordered_map<KeyCode, bool> m_currentKeyState;
  std::unordered_map<KeyCode, bool> m_previousKeyState;
  glm::vec2 m_mousePosition;
  glm::vec2 m_mouseDelta;
  
  std::unordered_map<MouseButton, bool>  m_mouseButtonState;
  std::unordered_map<MouseButton, bool>  m_prevMouseButtonState;
  float m_mouseScrollY;

  bool m_uiMouseCaptured{false};
  bool m_uiKbdCaptured{false};
  friend class Window; 
};

} // namespace nova