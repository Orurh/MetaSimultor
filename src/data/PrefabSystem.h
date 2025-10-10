#pragma once 
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>
#include "../core/agent.h"
namespace engine::data {

    struct AgentPrefab {
        std::string name;
        AgentType type = AgentType::TREE;

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
            void LoadPrefabs(const std::string& config_path);
            void SavePrefab(const std::string& name, const AgentPrefab& prefab);

            const AgentPrefab& GetPrefab(const std::string& name) const;
            bool PrefabExists(const std::string& name) const;

            Agent CreateAgentFromPrefab(const std::string& prefab_name, const glm::vec3& position);
            Agent CreateAgentFromPrefab(const AgentPrefab& prefab, const glm::vec3& position);

            std::vector<std::string> GetPrefabNames() const;
            std::vector<std::string> GetPrefabsByType(AgentType type) const;
        private:
            std::unordered_map<std::string, AgentPrefab> m_prefabs;
    };
}