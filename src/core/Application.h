#pragma once
#include "core/snapshot/SceneRecorder.h"
#include "ui/Editor.h"
#include <memory>
#include <string>
#include <vector>

namespace ui {
struct EditorContext;
}  // namespace ui
struct IAppExtension;

struct GLFWwindow;  // fwd, если используешь GLFW

namespace engine {
class SceneRecorder;
class Window;
class Scene;
namespace data {
class SceneSerializer;
}  // namespace data

class Application {
  public:
  Application(int width, int height, const char *title);
  ~Application();

  void Run();
  void Shutdown();
  // Расширения (плагины)
  void AddExtension(std::unique_ptr<IAppExtension> ext);

  // Геттеры для редакторского контекста
  [[nodiscard]] GLFWwindow *GetNativeWindow() const;
  [[nodiscard]] int GetViewportWidth() const;
  [[nodiscard]] int GetViewportHeight() const;
  [[nodiscard]] float GetDeltaTime() const;
  Scene &GetScene();
  SceneRecorder *GetSceneRecorder() { return m_sceneRecorder.get(); }

  bool SaveScene(const std::string &path);
  void LoadScene(const std::string &path);

  private:
  void InitGL();       // создать окно, контекст, glad, viewport и т.п.
  void TickCore();     // обновить dt, инпут
  void UpdateScene();  // m_scene.Update(dt)
  void RenderScene();  // m_scene.Render()
  void SwapBuffers();  // swap buffers
  [[nodiscard]] bool ShouldClose() const;

  std::unique_ptr<Window> m_window;
  std::shared_ptr<Scene> m_scene;
  std::unique_ptr<SceneRecorder> m_sceneRecorder;
  ui::Editor m_editor;
  float m_dt{0.F};
  float m_last_frame_time{0.F};
  int m_width{0}, m_height{0};

  std::unique_ptr<data::SceneSerializer> m_serializer;
  std::vector<std::unique_ptr<IAppExtension>> m_extensions;
};

}  // namespace engine
