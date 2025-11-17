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
  float m_local_spread_strength = 20.0F;

  // На будущее, под физику:
  float m_humidity_ignition_factor = 1.5F;  // как влажность повышает порог воспламенения
  float m_humidity_heat_dampen = 0.8F;      // как влажность гасит теплопередачу
  float m_wind_heat_bonus = 2.0F;           // усиление тепла по ветру
  float m_base_conductivity = 0.1F;         // базовая теплопроводность материала
  float m_max_heat_distance = 8.0F;         // макс. расстояние для теплопередачи
  float m_air_humidity_factor = 0.3F;       // глоб влажность воздуха
  
};
