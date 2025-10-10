#include "PrefabSystem.h"
#include <set>
#include <algorithm>
#include <stdexcept>
namespace engine::data {
   
    void PrefabSystem::SavePrefab(const std::string& name, const AgentPrefab &prefab)
    {
        m_prefabs[name] = prefab;
    }
    const AgentPrefab &PrefabSystem::GetPrefab(const std::string &name) const
    {
        auto it = m_prefabs.find(name);
        if (it == m_prefabs.end()) {
            throw std::runtime_error("Prefab not found: " + name);
        }
        return it->second;
    }
    bool PrefabSystem::PrefabExists(const std::string &name) const
    {
        return m_prefabs.count(name);
    }

    Agent PrefabSystem::CreateAgentFromPrefab(const std::string &prefab_name, const glm::vec3 &position)
    {
        const AgentPrefab& prefab = m_prefabs[prefab_name];
        return CreateAgentFromPrefab(prefab, position);
    }

    Agent PrefabSystem::CreateAgentFromPrefab(const AgentPrefab &prefab, const glm::vec3 &position)
    {
        Agent agent;
        agent.position = Float3::fromGlm(position);
        agent.fire_collider.bounds_min = Float3::fromGlm(prefab.bounds_min); 
        agent.fire_collider.bounds_max = Float3::fromGlm(prefab.bounds_max);
        
        agent.fire_collider.point_count = std::min(static_cast<int>(prefab.fire_points.size()), 64);

        for(int i = 0; i < agent.fire_collider.point_count; ++i) {
            Float3 local_pos = Float3::fromGlm(prefab.fire_points[i].position);
            agent.fire_collider.heat_points[i] = Float3(
                position.x + local_pos.x,
                position.y + local_pos.y, 
                position.z + local_pos.z
            );
            agent.fire_collider.fuels[i] = prefab.fire_points[i].initial_fuel;
            agent.fire_collider.intensities[i] = 0.0f;
        }

        agent.intencity = prefab.physics.intencity;
        agent.type = prefab.type;

        return agent;
    }

    std::vector<std::string> PrefabSystem::GetPrefabNames() const
    {
        std::vector<std::string> result;
        result.reserve(m_prefabs.size());
        std::transform(m_prefabs.begin(), m_prefabs.end(), std::back_inserter(result),
         [](const auto& pair){ return pair.first; });
        return result;
    }
    std::vector<std::string> PrefabSystem::GetPrefabsByType(AgentType type) const
    {
        std::vector<std::string> prefabs;

        for(auto [name, prefab]: m_prefabs) {
            if(prefab.type == type) {
                prefabs.push_back(name);
            }
        }

        return prefabs;
    }
}
