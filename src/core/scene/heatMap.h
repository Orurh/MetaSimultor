#pragma once

#include <memory>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>

#include "core/agent.h"
#include "graphics/Texture.h"

namespace engine {

class RiskPredictor; // forward

// Какой режим сейчас отображаем
enum class HeatMapMode {
  Temperature, // исходная температурная карта
  Risk         // карта риска (прогноз от RiskPredictor)
};

class HeatMap {
public:
  struct Grid {
    int m_width  = 0;
    int m_height = 0;

    float m_worldMinX = 0.0F;
    float m_worldMaxX = 0.0F;
    float m_worldMinZ = 0.0F;
    float m_worldMaxZ = 0.0F;

    // Температурная карта: в Кельвинах, либо отрицательное значение для выгоревших.
    std::vector<float> m_cells;

    // Карта риска: 0..1 (0 – нет риска, 1 – максимальный)
    std::vector<float> m_risk;

    void Resize(int w, int h);
  };

  struct Stats {
    float m_ambientT      = 0.0F; // температура среды (К)
    float m_minCellT      = 0.0F; // минимум по ячейкам (К, без выгоревших)
    float m_maxCellT      = 0.0F; // максимум по ячейкам (К, без выгоревших)
    int   m_cooledPoints  = 0;    // сколько точек дали ощутимое охлаждение
    int   m_burnedAgents  = 0;    // сколько агентов полностью выгорели

    // --- риск по температуре (черновой, только T, без ветра) ---
    float m_meanDeltaC      = 0.0F; // среднее (Tcell - Tamb) по карте
    float m_maxDeltaC       = 0.0F; // максимум (Tcell - Tamb)
    float m_warmArea01      = 0.0F; // доля ячеек с T > ambient + warmThr
    float m_criticalArea01  = 0.0F; // доля ячеек с T > ambient + criticalThr
  };

  HeatMap() = default;

  void Init(int width, int height);

  // Настройка world-границ, в которые вписываем XZ-координаты
  void SetWorldBounds(float minX, float maxX, float minZ, float maxZ);

  // Обновляем температурную карту по состоянию агентов
  void UpdateFromAgents(const std::vector<Agent> &agents,
                        float ambientTemperatureK);

  // Обновляем risk-grid на основе уже посчитанного RiskPredictor'ом
  void UpdateRiskPrediction(const RiskPredictor& predictor);

  [[nodiscard]] std::shared_ptr<Texture> GetTexture() const {
    return m_texture;
  }
  [[nodiscard]] Stats GetStats() const { return m_stats; }

  [[nodiscard]] glm::vec2 GetWorldMinXZ() const {
    return {m_grid.m_worldMinX, m_grid.m_worldMinZ};
  }

  [[nodiscard]] glm::vec2 GetWorldMaxXZ() const {
    return {m_grid.m_worldMaxX, m_grid.m_worldMaxZ};
  }

  [[nodiscard]] const Grid& GetGrid() const { return m_grid; }

  // Режим отображения
  void SetMode(HeatMapMode mode) { m_mode = mode; }
  [[nodiscard]] HeatMapMode GetMode() const { return m_mode; }

  // пока оставим, хотя для RiskPredictor ветер считается в нём самом
  void SetWind(const glm::vec3& windWorld) {
    glm::vec2 dir(windWorld.x, windWorld.z);
    float len = glm::length(dir);
    if (len > 1e-3f) {
      m_windDirXZ     = dir / len;
      m_windStrength  = len; // скорость, м/с
    } else {
      m_windDirXZ     = glm::vec2(0.0f);
      m_windStrength  = 0.0f;
    }
  }

private:
  Grid                      m_grid;
  Stats                     m_stats{};
  std::shared_ptr<Texture>  m_texture;
  HeatMapMode               m_mode = HeatMapMode::Temperature;

  // --- блок про ветер (может пригодиться для других визуализаций,
  // сейчас для RiskPredictor не используется напрямую) ---
  glm::vec2 m_windDirXZ{0.0f, 0.0f}; // нормализованное направление ветра по XZ
  float     m_windStrength = 0.0f;    // модуль ветра (м/с)

  // ------------- helpers -------------

  static int WorldToGrid(float x, float worldMin, float worldMax, int size);

  // Градиент "холодно → горячо" по температуре:
  // 0   – тёмно-синий,
  // 0.25– циан,
  // 0.5 – зелёный,
  // 0.75– жёлто-оранжевый,
  // 1   – ярко-красный.
  static glm::vec3 ColorFromHeat01(float t);

  // Градиент для риска:
  // 0       – почти нет риска → тёмный
  // 0..0.33 – низкий          → зелёный
  // 0.33..0.66 – средний      → жёлтый
  // 0.66..0.9 – высокий       → оранжевый
  // 0.9..1   – критический    → красный
  static glm::vec3 ColorFromRisk01(float r);

  // ------------- построение температурной карты -------------

  void BuildGridFromAgents(const std::vector<Agent> &agents,
                           float ambientTemperatureK);

  // ------------- заливка текстуры -------------

  void UploadToTexture();

  void UploadTemperatureToTexture();

  void UploadRiskToTexture();
};

} // namespace engine
