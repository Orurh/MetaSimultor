#include "PrefabSystem.h"
#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs = std::filesystem;
namespace engine::data {
   
    void PrefabSystem::SavePrefab(const std::string& name, const AgentPrefab &prefab)
    {
        m_prefabs[name] = prefab;
    }
    void PrefabSystem::SavePrefabToFile(const std::string &prefab_name, const std::string &file_path)
    {
        if (!PrefabExists(prefab_name)) {
            throw std::runtime_error("Prefab not found: " + prefab_name);
        }

        const AgentPrefab& prefab = GetPrefab(prefab_name);

        YAML::Node root;
        root["name"] = prefab_name;
        SerializePrefab(prefab, root);

        fs::path path(file_path);
        fs::create_directories(path.parent_path());

        std::ofstream file(file_path);
        file << root;

        std::cout << "Saved prefab '" << prefab_name << "' to " << file_path << std::endl;
    }
    void PrefabSystem::LoadPrefabFromFile(const std::string& file_path) {
        if (!fs::exists(file_path)) {
            throw std::runtime_error("File not found: " + file_path);
        }

        YAML::Node root = YAML::LoadFile(file_path);

        std::string name = root["name"].as<std::string>();
        AgentPrefab prefab = DeserializePrefab(root);

        m_prefabs[name] = prefab;
        std::cout << "Loaded prefab '" << name << "' from " << file_path << std::endl;
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
        agent.fire_collider.bounds_min = Float3::fromGlm(prefab.bounds_min + position); 
        agent.fire_collider.bounds_max = Float3::fromGlm(prefab.bounds_max + position);
        
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

    void PrefabSystem::CreatePrefabFromAgent(const std::string &name, const Agent &agent, const std::string &model_path, const glm::vec3& scale)
    {
        AgentPrefab prefab;
        prefab.name = name;
        prefab.type = agent.type;
        prefab.model_path = model_path;
        prefab.model_scale = scale;
        prefab.bounds_min = agent.fire_collider.bounds_min.toGlm();
        prefab.bounds_max = agent.fire_collider.bounds_max.toGlm();
        
        for (int i = 0; i < agent.fire_collider.point_count; i++) {
            AgentPrefab::FirePoint fp;
            fp.position = glm::vec3(
                agent.fire_collider.heat_points[i].x - agent.position.x,
                agent.fire_collider.heat_points[i].y - agent.position.y,
                agent.fire_collider.heat_points[i].z - agent.position.z
            );
            fp.initial_fuel = agent.fire_collider.fuels[i];
            prefab.fire_points.push_back(fp);
        }
        
        prefab.physics.intencity = agent.intencity;
        
        m_prefabs[name] = prefab;
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
    void PrefabSystem::SerializePrefab(const AgentPrefab &prefab, YAML::Node &node) const
    {
        node["type"] = static_cast<int>(prefab.type);
        node["model_path"] = prefab.model_path;
        node["model_scale"] = std::vector<float>{
            prefab.model_scale.x, 
            prefab.model_scale.y, 
            prefab.model_scale.z
        };
        node["bounds_min"] = std::vector<float>{prefab.bounds_min.x, prefab.bounds_min.y, prefab.bounds_min.z};
        node["bounds_max"] = std::vector<float>{prefab.bounds_max.x, prefab.bounds_max.y, prefab.bounds_max.z};
        
        node["physics"]["intensity"] = prefab.physics.intencity;
        
        for (const auto& fp : prefab.fire_points) {
            YAML::Node fp_node;
            fp_node["position"] = std::vector<float>{fp.position.x, fp.position.y, fp.position.z};
            fp_node["initial_fuel"] = fp.initial_fuel;
            node["fire_points"].push_back(fp_node);
        }

        // Visuals
        node["visuals"]["normal_material"] = prefab.visuals.normal_material;
        node["visuals"]["burning_material"] = prefab.visuals.burning_material;
        node["visuals"]["burned_material"] = prefab.visuals.burned_material;
    }
    AgentPrefab PrefabSystem::DeserializePrefab(const YAML::Node &node) const
    {
        AgentPrefab prefab;
        
        // Основные поля
        prefab.type = static_cast<AgentType>(node["type"].as<int>());
        prefab.model_path = node["model_path"].as<std::string>();
        if (node["model_scale"]) {
            auto scale = node["model_scale"].as<std::vector<float>>();
            prefab.model_scale = glm::vec3(scale[0], scale[1], scale[2]);
        } else {
            prefab.model_scale = glm::vec3(1.0f); 
        }
        // Bounds
        auto bounds_min = node["bounds_min"].as<std::vector<float>>();
        auto bounds_max = node["bounds_max"].as<std::vector<float>>();
        prefab.bounds_min = glm::vec3(bounds_min[0], bounds_min[1], bounds_min[2]);
        prefab.bounds_max = glm::vec3(bounds_max[0], bounds_max[1], bounds_max[2]);
        
        // Physics
        prefab.physics.intencity = node["physics"]["intensity"].as<float>();
        
        // Fire points
        if (node["fire_points"]) {
            for (const auto& fp_node : node["fire_points"]) {
                AgentPrefab::FirePoint fp;
                auto pos = fp_node["position"].as<std::vector<float>>();
                fp.position = glm::vec3(pos[0], pos[1], pos[2]);
                fp.initial_fuel = fp_node["initial_fuel"].as<float>();
                prefab.fire_points.push_back(fp);
            }
        }

        // Visuals
        prefab.visuals.normal_material = node["visuals"]["normal_material"].as<std::string>();
        prefab.visuals.burning_material = node["visuals"]["burning_material"].as<std::string>();
        prefab.visuals.burned_material = node["visuals"]["burned_material"].as<std::string>();

        return prefab;
}
}
