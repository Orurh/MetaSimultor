#include "WindPanel.h"
#include "core/scene/Scene.h"
#include "ui/EditorContext.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace ui {

void WindPanel::Draw(EditorContext &ctx) {
  if (!ctx.m_scene)
    return;

  auto &scene = *ctx.m_scene;
  auto &windConfig = scene.GetWindConfig();
  auto &fireConfig = scene.GetFireModule();
  auto &env = fireConfig.Config().m_env;

  ImGui::Begin("Wind, Temp & Environment", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

  // -----------------------
  // WIND
  // -----------------------
  ImGui::TextUnformatted("Wind");
  ImGui::Separator();

  // Направление ветра
  ImGui::Text("Direction");
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
  ImGui::Checkbox("Show Wind Visualization", &m_showWindVisualization);

  auto currentWind = windConfig.getCurrentWind(scene.GetWindTime());
  ImGui::Text("Current Wind: (%.1f, %.1f, %.1f)", currentWind.x, currentWind.y, currentWind.z);
  ImGui::Text("Base Wind: (%.1f, %.1f, %.1f)", windConfig.direction.x * windConfig.strength,
              windConfig.direction.y * windConfig.strength, windConfig.direction.z * windConfig.strength);
  // -----------------------
  // ENVIRONMENT / FIRE
  // -----------------------
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::TextUnformatted("Environment / Fire");
  ImGui::Separator();

  // Температура окружающей среды
  {
    float ambient = env.m_ambient_temperature;
    ImGui::Text("Ambient temp (K)");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderFloat("##AmbientTemp", &ambient, 250.0f, 800.0f, "%.1f")) {
      env.m_ambient_temperature = ambient;
    }
  }

  // Влажность воздуха
  {
    float airHum = env.m_air_humidity;
    ImGui::Text("Air humidity");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderFloat("##AirHumidity", &airHum, 0.0f, 1.0f, "%.2f")) {
      env.m_air_humidity = airHum;
    }
  }

  // Глобальный множитель сырости сцены
  {
    float globalMoist = env.m_global_moisture_scale;
    ImGui::Text("Global moisture x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderFloat("##GlobalMoisture", &globalMoist, 0.0f, 2.0f, "%.2f")) {
      env.m_global_moisture_scale = globalMoist;
    }
  }

  // (опционально) насколько влажность гасит теплопередачу
  {
    float damp = env.m_humidity_dampen_coeff;
    ImGui::Text("Humidity damp");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderFloat("##HumidityDamp", &damp, 0.0f, 2.0f, "%.2f")) {
      env.m_humidity_dampen_coeff = damp;
    }
  }

  // (опционально) насколько влажность поднимает порог воспламенения
  {
    float ignMult = env.m_humidity_ignition_mult;
    ImGui::Text("Humidity -> ignition");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderFloat("##HumidityIgnMult", &ignMult, 0.0f, 3.0f, "%.2f")) {
      env.m_humidity_ignition_mult = ignMult;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Кнопка сброса
  if (ImGui::Button("Reset to Default")) {
    windConfig = WindConfig();

    // сбрасываем базовую среду к дефолтам из FireEnvironmentConfig
    env.m_ambient_temperature = 293.15f;
    env.m_air_humidity = 0.5f;
    env.m_humidity_dampen_coeff = 0.8f;
    env.m_humidity_ignition_mult = 0.5f;
    env.m_wind_speed = 5.0f;
    env.m_global_moisture_scale = 1.0f;

    // локальные кэши ветра
    m_direction[0] = 1.0f;
    m_direction[1] = 0.0f;
    m_direction[2] = 0.0f;
    m_strength = 10.0f;
    m_gustiness = 0.3f;
    m_changeSpeed = 0.1f;
  }

  ImGui::End();
}

}  // namespace ui
