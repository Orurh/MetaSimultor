#pragma once
#include <string>
#include "Window.h"
#include "scene/Scene.h"
namespace engine {
    class Application {
        public:
            Application(int width, int height, const std::string& title);
            void Run();
        private:
            Window m_window;
            Scene m_scene;
            float m_lastFrameTime;
    };
}