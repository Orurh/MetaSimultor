#pragma once

// Набор "готовых" коэффициентов для CUDA-ядра.
// Чистый POD без CUDA-зависимостей.
struct FireSimulationParams {
  // Поиск соседних агентов (объём, в котором смотрим)
  float m_base_check_distance = 15.0F;

  // Радиус распространения от точки к точке между агентами
  float m_base_spread_distance = 3.0F;

  // Радиус распространения внутри одного агента (крона/ствол и т.п.)
  float m_local_spread_distance = 2.0F;

  // Насколько сильно огонь перекидывается между агентами
  float m_cross_agent_spread_strength = 10.0F;

  // Насколько сильно огонь “растекается” по точкам внутри одного агента
  float m_local_spread_strength = 15.0F;

  // --- NEW: начальный буст для холодных материалов ---
  // Увеличить начальную скорость распространения между ещё холодными объектами
  float m_initial_spread_boost = 2.0F;

  // Штраф (чуть душим совсем холодные материалы, чтобы не улететь в космос)
  // 1.0 — без штрафа, < 1.0 — немного ослабляем.
  float m_cold_material_penalty = 0.7F;

  // На будущее, под физику:
  float m_humidity_ignition_factor = 1.5F;  // как влажность повышает порог воспламенения
  float m_humidity_heat_dampen = 0.8F;      // как влажность гасит теплопередачу
  float m_wind_heat_bonus = 2.0F;           // усиление тепла по ветру
  float m_base_conductivity = 0.1F;         // базовая теплопроводность материала
  float m_max_heat_distance = 8.0F;         // макс. расстояние для теплопередачи
  float m_air_humidity_factor = 0.3F;       // глоб влажность воздуха
  float m_ambient_temperature = 293.15F;    // ~20°C по умолчанию
  float m_global_moisture_scale = 1.0F;     // глобальный множитель сырости

  // параметры горения / охлаждения
  // Порог, выше которого точка считается "горящей"
  float m_fire_intensity_min = 0.05F;  // было 0.1F

  // Порог интенсивности, чтобы точка могла распространять огонь
  float m_spread_intensity_threshold = 0.15F;  // было 0.3F

  // Максимальная интенсивность
  float m_max_intensity = 1.0F;

  // Скорость сжигания топлива
  float m_burn_fuel_rate = 0.2F;

  // Скорость роста интенсивности
  float m_intensity_growth_rate = 2.0F;

  // Нагрев: во сколько раз интенсивность умножается в приращении температуры
  float m_heat_production = 150.0F;

  // Скорость охлаждения к окружающей среде
  float m_cooling_rate = 10.0F;

  // Порог топлива, ниже которого считаем, что топлива "нет"
  float m_fuel_epsilon = 0.1F;

  // максимальная температура горения леса пока что
  float m_max_temperature = 1300.0F;
};
