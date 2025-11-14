// core/FireConfig.h
#pragma once

#include "FireSimulationParams.h"
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

// Параметры "материала" префаба (дерево, трава, дом и т.п.)
struct FireMaterialParams {
  float m_ignition_temperature = 600.0F; // °C или K — главное, везде одинаково
  float m_fuel_density         = 100.0F; // условный запас топлива на точку
  float m_conductivity         = 0.1F;   // теплопроводность материала
  float m_base_moisture        = 0.3F;   // типичная влажность 0..1
};

// Параметры окружающей среды (глобальные, одна на всю сцену)
struct FireEnvironmentConfig {
  float     m_ambient_temperature = 293.15F; // ~20°C
  float     m_air_humidity        = 0.3F;    // влажность воздуха 0..1
  glm::vec3 m_wind_direction      = {1.0F, 0.0F, 0.0F};
  float     m_wind_speed          = 5.0F;    // м/с, условно
};

// Общий конфиг огня на CPU
struct FireConfig {
  FireEnvironmentConfig m_env;

  // Материалы по строковому id: "wood", "grass", "house", "stone" и т.п.
  std::unordered_map<std::string, FireMaterialParams> m_materials;

  // Какой материал считать дефолтным, если id не найден
  std::string m_default_material_id = "wood";

  FireConfig();  // задаёт дефолтные материалы wood/grass/house

  // Удобный доступ по строковому id
  FireMaterialParams& Material(const std::string& id);
  const FireMaterialParams& Material(const std::string& id) const;
};

// Построение GPU-параметров для ядра из доменного конфига.
void BuildSimulationParams(const FireConfig& cfg,
                           FireSimulationParams& out_params);
