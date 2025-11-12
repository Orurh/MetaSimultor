#include "WindPanel.h"
#include "ui/EditorContext.h"
#include "core/scene/Scene.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace ui {

void WindPanel::Draw(EditorContext& ctx) {
    if (!ctx.m_scene) return;
    
    auto& scene = *ctx.m_scene;
    auto& windConfig = scene.GetWindConfig();
    
    ImGui::Begin("Wind Configuration", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    // Направление ветра
    ImGui::Text("Wind Direction");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::DragFloat3("##Direction", m_direction, 0.1f, -1.0f, 1.0f)) {
        windConfig.direction = glm::vec3(m_direction[0], m_direction[1], m_direction[2]);
    }
    
    // Сила ветра
    ImGui::Text("Strength");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderFloat("##Strength", &m_strength, 0.0f, 50.0f, "%.1f")) {
        windConfig.strength = m_strength;
    }
    
    // Порывистость
    ImGui::Text("Gustiness");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderFloat("##Gustiness", &m_gustiness, 0.0f, 1.0f, "%.2f")) {
        windConfig.gustiness = m_gustiness;
    }
    
    // Скорость изменения
    ImGui::Text("Change Speed");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderFloat("##ChangeSpeed", &m_changeSpeed, 0.0f, 1.0f, "%.2f")) {
        windConfig.change_speed = m_changeSpeed;
    }
    
    ImGui::Separator();
    
    // Визуализация
    ImGui::Checkbox("Show Wind Visualization", &m_showWindVisualization);
    
    // Текущий ветер
    auto currentWind = windConfig.getCurrentWind(scene.GetWindTime()); 
    ImGui::Text("Current Wind: (%.1f, %.1f, %.1f)", 
                currentWind.x, currentWind.y, currentWind.z);
    
    // Кнопка сброса
    if (ImGui::Button("Reset to Default")) {
        windConfig = WindConfig();
        m_direction[0] = 1.0f; m_direction[1] = 0.0f;
        m_strength = 10.0f;
        m_gustiness = 0.3f;
        m_changeSpeed = 0.1f;
    }
    
    ImGui::End();
}

} // namespace ui