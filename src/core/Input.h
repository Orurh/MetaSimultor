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
  glm::vec2 GetMouseDelta() const { return m_mouseDelta; }
  bool GetMouseButton(int button, InputState state = InputState::Held) const;
  void SetMousePosition(float x, float y);
  void SetMouseButtonState(int button, bool pressed);
  float GetMouseScrollY() const { return m_mouseScrollY; }
  void SetMouseScrollY(float yoffset) { m_mouseScrollY = yoffset; }
  void ResetMouseScroll() { m_mouseScrollY = 0.0f; }
private:
  std::unordered_map<KeyCode, bool> m_currentKeyState;
  std::unordered_map<KeyCode, bool> m_previousKeyState;
  glm::vec2 m_mousePosition;
  glm::vec2 m_mouseDelta;
  
  std::unordered_map<int, bool> m_mouseButtonState;
  std::unordered_map<int, bool> m_prevMouseButtonState;
  float m_mouseScrollY;
  friend class Window; 
};

} // namespace nova