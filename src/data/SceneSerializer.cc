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

            out << YAML::Key << "metadata" << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "version" << YAML::Value << "1.0";
            out << YAML::Key << "date" << YAML::Value << getCurrentDateTime();
            out << YAML::EndMap;

            out << YAML::Key << "agents" << YAML::Value;
            out << YAML::BeginSeq;
            auto agents = m_scene->LoadAgentsFromGPU();
            for (int i = 0; i < agents.size(); i++) {
                SerializeAgent(out, agents[i], i);
            }
            out << YAML::EndSeq;
        
            out << YAML::EndMap;
            
            std::ofstream file(filepath);
            file << out.c_str();
            std::cout << "Scene saved to: " << filepath << std::endl;
        }   catch (const std::exception& e) {
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

    void SceneSerializer::Deserialize(const std::string& filename) {
        
    }
}
