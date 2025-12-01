// src/main/main.cpp
#include "app/EditorExtension.h"
#include "core/Application.h"

int main() {
  engine::Application app(1920, 1080, "Sim");

  app.AddExtension(std::make_unique<EditorExtension>());  // ← редактор
  // app.AddExtension(std::make_unique<PerfOverlayExtension>()); // ← потом
  // добавишь любые

  app.Run();
  app.Shutdown();
  return 0;
}
