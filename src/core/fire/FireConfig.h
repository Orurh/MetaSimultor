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
  float m_max_temperature      = 1300.0F;
};

// Параметры окружающей среды (глобальные, одна на всю сцену)
struct FireEnvironmentConfig {
  float m_ambient_temperature    = 293.15F; // ~20°C
  float m_air_humidity           = 0.5F;    // влажность воздуха 0..1
  float m_humidity_dampen_coeff  = 0.8F;    // как влажность гасит теплопередачу
  float m_humidity_ignition_mult = 0.5F;    // как влажность поднимает порог
  float m_wind_speed             = 5.0F;    // м/с, условно
  float m_global_moisture_scale  = 1.0F;
};

// Общий конфиг огня на CPU
struct FireConfig {
  FireEnvironmentConfig m_env;  // ← ОДНО поле

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
