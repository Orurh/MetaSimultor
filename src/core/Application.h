#pragma once
#include <string>
#include "Window.h"
#include "scene/Scene.h"
#include "data/SceneSerializer.h"
namespace engine {
    class Application {
        public:
            Application(int width, int height, const std::string& title);
            void Run();
            void SaveScene(const std::string& path);
    
            void LoadScene(const std::string& path);
        private:
            Window m_window;
            Scene m_scene;
            float m_last_frame_time;
            std::unique_ptr<data::SceneSerializer> m_serializer;
    };
}