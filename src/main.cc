// src/main/main.cpp
#include "core/Application.h"
#include "app/EditorExtension.h"

int main() {
    engine::Application app(1920, 1080, "Sim");

    app.AddExtension(std::make_unique<EditorExtension>()); // ← редактор
    // app.AddExtension(std::make_unique<PerfOverlayExtension>()); // ← потом добавишь любые

    app.Run();
    return 0;
}
