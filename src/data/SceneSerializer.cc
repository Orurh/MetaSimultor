#include "SceneSerializer.h"
#include "core/scene/Scene.h"
#include <filesystem>
#include <fstream>

#include <iostream>
#include <string>
namespace engine::data{
    SceneSerializer::SceneSerializer(std::shared_ptr<Scene> scene) : m_scene(scene) {

    }

    // void SceneSerializer::Serialize(const std::string& filepath) {
    //     try {
    //         YAML::Emitter out;
    //         out << YAML::BeginMap;

    //         out << YAML::Key << "metadata" << YAML::Value;
    //         out << YAML::BeginMap;
    //         out << YAML::Key << "version" << YAML::Value << "1.0";
    //         out << YAML::Key << "date" << YAML::Value << getCurrentDateTime();
    //         out << YAML::EndMap;

    //         out << YAML::Key << "agents" << YAML::Value;
    //         out << YAML::BeginSeq;
    //         auto agents = m_scene->LoadAgentsFromGPU();
    //         for (int i = 0; i < agents.size(); i++) {
    //             SerializeAgent(out, agents[i], i);
    //         }
    //         out << YAML::EndSeq;
        
    //         out << YAML::EndMap;
            
    //         std::ofstream file(filepath);
    //         file << out.c_str();
    //         std::cout << "Scene saved to: " << filepath << std::endl;
    //     }   catch (const std::exception& e) {
    //         std::cerr << "Failed to serialize scene: " << e.what() << std::endl;
    //     }
    // }

    bool SceneSerializer::Serialize(const std::string& filepath) {
    try {
        // 0) Диагностика рабочей директории
        std::cout << "[Serialize] CWD: " << std::filesystem::current_path() << "\n";

        // 1) Собираем YAML
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "metadata" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "version" << YAML::Value << "1.0";
        out << YAML::Key << "date"    << YAML::Value << getCurrentDateTime();
        out << YAML::EndMap;

        out << YAML::Key << "agents" << YAML::Value;
        out << YAML::BeginSeq;
        auto agents = m_scene->LoadAgentsFromGPU();
        for (int i = 0; i < static_cast<int>(agents.size()); ++i) {
            SerializeAgent(out, agents[i], i);
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        // 2) Гарантируем, что директория есть
        std::filesystem::path p(filepath);
        if (p.has_parent_path() && !p.parent_path().empty()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
            if (ec) {
                std::cerr << "[Serialize] create_directories failed for '"
                          << p.parent_path() << "': " << ec.message() << "\n";
                // продолжаем — возможно папка всё же есть
            }
        }

        // 3) Открываем файл с проверкой + исключения
        std::ofstream file;
        file.exceptions(std::ios::badbit); // badbit -> исключение, failbit проверим сами
        file.open(filepath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            std::cerr << "[Serialize] cannot open for write: " << filepath << "\n";
            return false;
        }

        // 4) Пишем буфер и проверяем статус
        const char* data = out.c_str();
        const std::size_t n = out.size();
        file.write(data, static_cast<std::streamsize>(n));
        file.flush();
        if (!file.good()) {
            std::cerr << "[Serialize] write failed: " << filepath << "\n";
            return false;
        }

        std::cout << "[Serialize] Scene saved to: "
                  << std::filesystem::absolute(p) << " (" << n << " bytes)\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Serialize] exception: " << e.what() << "\n";
        return false;
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
