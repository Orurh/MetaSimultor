#include "SceneSerializer.h"
#include "core/scene/Scene.h"
#include <fstream>

#include <iostream>
#include <string>
namespace engine::data{
    SceneSerializer::SceneSerializer(std::shared_ptr<Scene> scene) : m_scene(scene) {

    }

    void SceneSerializer::Serialize(const std::string& filepath) {
        try {
        YAML::Emitter out;
        out << YAML::BeginMap;

        // Метаданные
        out << YAML::Key << "metadata" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "version" << YAML::Value << "1.0";
        out << YAML::Key << "date" << YAML::Value << getCurrentDateTime();
        out << YAML::EndMap;

        // Агенты
        out << YAML::Key << "agents" << YAML::Value;
        out << YAML::BeginSeq;
        auto agents = m_scene->LoadAgentsFromGPU();
        for (int i = 0; i < agents.size(); i++) {
            SerializeAgent(out, agents[i], i);
        }
        out << YAML::EndSeq;

        // SceneObject'ы
        out << YAML::Key << "scene_objects" << YAML::Value;
        out << YAML::BeginSeq;
        for (int i = 0; i < m_scene->GetSceneObjects().size(); i++) {
            SerializeSceneObject(out, m_scene->GetSceneObjects()[i], i);
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream file(filepath);
        file << out.c_str();
        std::cout << "Scene saved to: " << filepath << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Failed to serialize scene: " << e.what() << std::endl;
    }
    }

    YAML::Emitter& SceneSerializer::SerializeFireCollider(YAML::Emitter& out, const FireCollider& collider) {
         {
            out << YAML::BeginMap;

            // Bounds
            out << YAML::Key << "bounds_min" << YAML::Value;
            out << YAML::Flow << YAML::BeginSeq << collider.bounds_min.x << collider.bounds_min.y << collider.bounds_min.z << YAML::EndSeq;

            out << YAML::Key << "bounds_max" << YAML::Value;
            out << YAML::Flow << YAML::BeginSeq << collider.bounds_max.x << collider.bounds_max.y << collider.bounds_max.z << YAML::EndSeq;

            out << YAML::Key << "point_count" << YAML::Value << collider.point_count;

            // Heat points (только используемые)
            out << YAML::Key << "heat_points" << YAML::Value;
            out << YAML::BeginSeq;
            for (int i = 0; i < collider.point_count; i++) {
                out << YAML::Flow << YAML::BeginSeq << collider.heat_points[i].x << collider.heat_points[i].y << collider.heat_points[i].z << YAML::EndSeq;
            }
            out << YAML::EndSeq;

            // Intensities
            out << YAML::Key << "intensities" << YAML::Value;
            out << YAML::Flow << YAML::BeginSeq;
            for (int i = 0; i < collider.point_count; i++) {
                out << collider.intensities[i];
            }
            out << YAML::EndSeq;

            // Fuels
            out << YAML::Key << "fuels" << YAML::Value;
            out << YAML::Flow << YAML::BeginSeq;
            for (int i = 0; i < collider.point_count; i++) {
                out << collider.fuels[i];
            }
            out << YAML::EndSeq;

            out << YAML::EndMap;
            return out;
          }
    }

    YAML::Emitter& SceneSerializer::SerializeAgent(YAML::Emitter& out, const Agent& agent, int id) {
        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << id;
        out << YAML::Key << "type" << YAML::Value << static_cast<int>(agent.type);
        out << YAML::Key << "object_id" << YAML::Value << agent.object_id;
        out << YAML::Key << "intensity" << YAML::Value << agent.intencity;

        out << YAML::Key << "position" << YAML::Value;
        out << YAML::Flow << YAML::BeginSeq << agent.position.x << agent.position.y << agent.position.z << YAML::EndSeq;

        // FireCollider
        out << YAML::Key << "fire_collider" << YAML::Value;
        SerializeFireCollider(out, agent.fire_collider);

        out << YAML::EndMap;
        return out;
    }

      YAML::Emitter& SceneSerializer::SerializeSceneObject(YAML::Emitter& out, const SceneObject& obj, int id) {
        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << id;
        out << YAML::Key << "agent_id" << YAML::Value << obj.agent_id;
        out << YAML::Key << "object_type" << YAML::Value << obj.object_type;
        out << YAML::Key << "is_visible" << YAML::Value << obj.is_visible;

        // Позиция и масштаб
        out << YAML::Key << "position" << YAML::Value;
        out << YAML::Flow << YAML::BeginSeq << obj.position.x << obj.position.y << obj.position.z << YAML::EndSeq;

        out << YAML::Key << "scale" << YAML::Value;
        out << YAML::Flow << YAML::BeginSeq << obj.scale.x << obj.scale.y << obj.scale.z << YAML::EndSeq;

        out << YAML::EndMap;
        return out;
    }

    void SceneSerializer::Deserialize(const std::string& filename) {
        try {
        YAML::Node scene_data = YAML::LoadFile(filename);
        
        // Очищаем сцену
        m_scene->Clear();
        
        // 1. Загружаем агентов
        const YAML::Node& agents_node = scene_data["agents"];
        std::vector<Agent> agents;
        
        for (const YAML::Node& agent_node : agents_node) {
            agents.push_back(DeserializeAgent(agent_node));
        }
        
        // Загружаем агентов в GPU
        m_scene->GetAgentManager().SetAgents(agents);
        
        const YAML::Node& objects_node = scene_data["scene_objects"];
        for (const YAML::Node& obj_node : objects_node) {
            SceneObject obj = DeserializeSceneObject(obj_node);
            m_scene->AddSceneObject(obj);
        }
        
        std::cout << "Scene loaded: " << agents.size() << " agents, " 
                  << objects_node.size() << " objects" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to deserialize scene: " << e.what() << std::endl;
    }
    }
    SceneObject SceneSerializer::DeserializeSceneObject(const YAML::Node& obj_node) {
    SceneObject obj;
    
    obj.agent_id = obj_node["agent_id"].as<int>();
    obj.object_type = obj_node["object_type"].as<std::string>();
    obj.is_visible = obj_node["is_visible"].as<bool>();
    
    // Позиция
    const YAML::Node& pos_node = obj_node["position"];
    obj.position = glm::vec3(
        pos_node[0].as<float>(),
        pos_node[1].as<float>(),
        pos_node[2].as<float>()
    );
    
    // Масштаб
    const YAML::Node& scale_node = obj_node["scale"];
    obj.scale = glm::vec3(
        scale_node[0].as<float>(),
        scale_node[1].as<float>(),
        scale_node[2].as<float>()
    );
    
    return obj;
}

     FireCollider SceneSerializer::DeserializeFireCollider(const YAML::Node& collider_node) {
            FireCollider collider;

            const YAML::Node& min_node = collider_node["bounds_min"];
            collider.bounds_min = Float3(
                min_node[0].as<float>(), 
                min_node[1].as<float>(), 
                min_node[2].as<float>()
            );

            const YAML::Node& max_node = collider_node["bounds_max"];
            collider.bounds_max = Float3(
                max_node[0].as<float>(), 
                max_node[1].as<float>(), 
                max_node[2].as<float>()
            );

            // Point count
            collider.point_count = collider_node["point_count"].as<int>();

            // Heat points
            const YAML::Node& points_node = collider_node["heat_points"];
            for (int i = 0; i < collider.point_count && i < points_node.size(); i++) {
                const YAML::Node& point_node = points_node[i];
                collider.heat_points[i] = Float3(
                    point_node[0].as<float>(),
                    point_node[1].as<float>(),
                    point_node[2].as<float>()
                );
            }

            // Intensities
            const YAML::Node& intensities_node = collider_node["intensities"];
            for (int i = 0; i < collider.point_count && i < intensities_node.size(); i++) {
                collider.intensities[i] = intensities_node[i].as<float>();
            }

            // Fuels
            const YAML::Node& fuels_node = collider_node["fuels"];
            for (int i = 0; i < collider.point_count && i < fuels_node.size(); i++) {
                collider.fuels[i] = fuels_node[i].as<float>();
            }

            return collider;
          }

        
          Agent SceneSerializer::DeserializeAgent(const YAML::Node& agent_node) {
            Agent agent;

            agent.type = static_cast<AgentType>(agent_node["type"].as<int>());
            agent.object_id = agent_node["object_id"].as<int>();
            agent.intencity = agent_node["intensity"].as<float>();

            const YAML::Node& pos_node = agent_node["position"];
            agent.position = Float3(
                pos_node[0].as<float>(),
                pos_node[1].as<float>(), 
                pos_node[2].as<float>()
            );

            if (agent_node["fire_collider"]) {
                agent.fire_collider = DeserializeFireCollider(agent_node["fire_collider"]);
            }

            return agent;
          }
}
