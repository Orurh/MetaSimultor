#pragma once 
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>
#include "core/agent.h"
#include <yaml-cpp/yaml.h>

namespace engine::data {

    struct AgentPrefab {
        std::string name;
        AgentType type = AgentType::TREE;

        glm::vec3 model_scale = glm::vec3(1.0f);

        std::string model_path;
        glm::vec3 bounds_min = {-1.0f, 0.0f, -1.0f};
        glm::vec3 bounds_max = {1.0f, 2.0f, 1.0f};
        
        struct FirePoint {
            glm::vec3 position;
            float initial_fuel = 100.0f;
        };
        std::vector<FirePoint> fire_points;

        struct Phisics {
            float intencity;
        } physics;

        struct Visuals {
            std::string normal_material;
            std::string burning_material; 
            std::string burned_material;
        } visuals;
    };

    class PrefabSystem {
        public: 
            void SavePrefab(const std::string& name, const AgentPrefab& prefab);
            void SavePrefabToFile(const std::string& prefab_name, const std::string& file_path);
            void LoadPrefabFromFile(const std::string& file_path);
            const AgentPrefab& GetPrefab(const std::string& name) const;
            bool PrefabExists(const std::string& name) const;

            Agent CreateAgentFromPrefab(const std::string& prefab_name, const glm::vec3& position);
            Agent CreateAgentFromPrefab(const AgentPrefab& prefab, const glm::vec3& position);
            void CreatePrefabFromAgent(const std::string& name, const Agent& agent, 
                              const std::string& model_path, const glm::vec3& scale = glm::vec3(1.0f));
    
            std::vector<std::string> GetPrefabNames() const;
            std::vector<std::string> GetPrefabsByType(AgentType type) const;
        private:
            void SerializePrefab(const AgentPrefab& prefab, YAML::Node& node) const;

            AgentPrefab DeserializePrefab(const YAML::Node& node) const;
            std::unordered_map<std::string, AgentPrefab> m_prefabs;
    };
}