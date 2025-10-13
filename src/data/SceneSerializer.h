#pragma once
#include "core/scene/Scene.h"
#include <chrono>
#include <yaml-cpp/emitterdef.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>
namespace engine::data {
    class SceneSerializer {
        public:
          SceneSerializer(std::shared_ptr<Scene> scene);

          bool Serialize(const std::string& path);
          
          void Deserialize(const std::string& path);
        private:    
          std::string getCurrentDateTime() {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);

            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
            return ss.str();
          }
          YAML::Emitter& SerializeAgent(YAML::Emitter& out, const Agent& agent, int id);

          YAML::Emitter& SerializeFireCollider(YAML::Emitter& out, const FireCollider& collider);
          std::shared_ptr<Scene> m_scene;
    };
}