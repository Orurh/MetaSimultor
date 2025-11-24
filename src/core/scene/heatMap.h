#pragma once

#include <limits>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "core/agent.h"
#include "graphics/Texture.h"

namespace engine {

class HeatMap {
public:
  struct Grid {
    int m_width = 0;
    int m_height = 0;

    float m_worldMinX = 0.0F;
    float m_worldMaxX = 0.0F;
    float m_worldMinZ = 0.0F;
    float m_worldMaxZ = 0.0F;

    // В Кельвинах, либо специальное отрицательное значение для выгоревших.
    std::vector<float> m_cells;

    void Resize(int w, int h) {
      m_width = w;
      m_height = h;
      m_cells.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0F);
    }
  };

  struct Stats {
    float m_ambientT = 0.0F; // температура среды (К)
    float m_minCellT = 0.0F; // минимум по ячейкам (К, без выгоревших)
    float m_maxCellT = 0.0F; // максимум по ячейкам (К, без выгоревших)
    int m_cooledPoints = 0;  // сколько точек дали ощутимое охлаждение
    int m_burnedAgents = 0;  // сколько агентов полностью выгорели
  };

  HeatMap() = default;

  void Init(int width, int height) {
    m_grid.Resize(width, height);

    TextureSpecification spec;
    spec.Width = static_cast<uint32_t>(width);
    spec.Height = static_cast<uint32_t>(height);
    spec.Format = ImageFormat::RGBA8;
    spec.GenerateMips = false;

    m_texture = Texture::Create(spec);
  }

  // Настройка world-границ, в которые вписываем XZ-координаты
  void SetWorldBounds(float minX, float maxX, float minZ, float maxZ) {
    m_grid.m_worldMinX = minX;
    m_grid.m_worldMaxX = maxX;
    m_grid.m_worldMinZ = minZ;
    m_grid.m_worldMaxZ = maxZ;
  }

  // Обновляем карту по состоянию агентов
  void UpdateFromAgents(const std::vector<Agent> &agents,
                        float ambientTemperatureK) {
    if (!m_texture) {
      Init(256, 256);
    }

    BuildGridFromAgents(agents, ambientTemperatureK);
    UploadToTexture();
  }

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

  [[nodiscard]] const Grid &GetGrid() const { return m_grid; }

private:
  Grid m_grid;
  Stats m_stats{};
  std::shared_ptr<Texture> m_texture;

  static int WorldToGrid(float x, float worldMin, float worldMax, int size) {
    const float len = worldMax - worldMin;
    if (len <= 1e-5f)
      return 0;

    float t = (x - worldMin) / len;
    int i = static_cast<int>(t * static_cast<float>(size));
    if (i < 0)
      i = 0;
    if (i >= size)
      i = size - 1;
    return i;
  }

  // Градиент "холодно → горячо":
  // 0   – тёмно-синий,
  // 0.25– циан,
  // 0.5 – зелёный,
  // 0.75– жёлто-оранжевый,
  // 1   – ярко-красный.
  static glm::vec3 ColorFromHeat01(float t) {
    t = glm::clamp(t, 0.0F, 1.0F);

    if (t < 0.25F) {
      float k = t / 0.25F;
      return glm::mix(glm::vec3(0.0F, 0.0F, 0.4F), glm::vec3(0.0F, 0.8F, 1.0F),
                      k); // dark blue → cyan
    }
    if (t < 0.5F) {
      float k = (t - 0.25F) / 0.25F;
      return glm::mix(glm::vec3(0.0F, 0.8F, 1.0F), glm::vec3(0.0F, 1.0F, 0.0F),
                      k); // cyan → green
    }
    if (t < 0.75F) {
      float k = (t - 0.5F) / 0.25F;
      return glm::mix(glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 0.8F, 0.0F),
                      k); // green → yellow/orange
    }
    float k = (t - 0.75F) / 0.25F;
    return glm::mix(glm::vec3(1.0F, 0.8F, 0.0F), glm::vec3(1.0F, 0.1F, 0.0F),
                    k); // orange → red
  }

  void BuildGridFromAgents(const std::vector<Agent> &agents,
                           float ambientTemperatureK) {
    const int W = m_grid.m_width;
    const int H = m_grid.m_height;
    if (W <= 0 || H <= 0) {
      return;
    }

    // Фон – чистая температура среды.
    m_grid.m_cells.assign(static_cast<size_t>(W) * H, ambientTemperatureK);

    float minCellT = std::numeric_limits<float>::max();
    float maxCellT = std::numeric_limits<float>::lowest();
    int cooledPoints = 0;
    int burnedAgents = 0;

    // Порог "горит/не горит" в °C
    constexpr float BURN_DELTA_C = 50.0F;
    // Максимальное охлаждение негорящих деревьев (в °C)
    constexpr float MAX_COOLING_C = 10.0F;
    constexpr int STAMP_RADIUS = 2;
    constexpr float AMBIENT_EPS_K = 1.0F; // было 0.25f — делаем помягче

    const float AMBIENT_C = ambientTemperatureK - 273.15F;

    for (const Agent &a : agents) {
      const FireCollider &c = a.fire_collider;
      const int N = c.point_count;
      if (N <= 0) {
        continue;
      }

      bool isBurnedAgent = (a.type == FireAgentState::Burned);
      if (isBurnedAgent) {
        ++burnedAgents;
      }

      for (int i = 0; i < N; ++i) {
        const Float3 &p = c.heat_points[i];
        float rawT = c.temperatures[i]; // К
        float M = glm::clamp(c.moisture[i], 0.0F, 1.0F);

        int gx = WorldToGrid(p.x, m_grid.m_worldMinX, m_grid.m_worldMaxX, W);
        int gz = WorldToGrid(p.z, m_grid.m_worldMinZ, m_grid.m_worldMaxZ, H);
        if (gx < 0 || gx >= W || gz < 0 || gz >= H) {
          continue;
        }

        const int IDX = (gz * W) + gx;

        if (isBurnedAgent) {
          m_grid.m_cells[IDX] = -100.0F;
          continue;
        }
        // Перевод в °C
        float rawC = rawT - 273.15F;
        float deltaC = rawC - AMBIENT_C;
        bool isBurningPoint = (deltaC > BURN_DELTA_C);
        float baseVisualT = rawT;

        if (!isBurningPoint) {
          // Негорящие деревья ВСЕГДА чуть холоднее воздуха.
          float factor = 0.3F + (0.7F * M);
          float coolingC = factor * MAX_COOLING_C;
          float biasC = -coolingC;

          baseVisualT = ambientTemperatureK + biasC;

          if (coolingC > 0.5f)
            ++cooledPoints;
        }

        for (int dz = -STAMP_RADIUS; dz <= STAMP_RADIUS; ++dz) {
          int nz = gz + dz;
          if (nz < 0 || nz >= H) {
            continue;
          }
          for (int dx = -STAMP_RADIUS; dx <= STAMP_RADIUS; ++dx) {
            int nx = gx + dx;
            if (nx < 0 || nx >= W) {
              continue;
            }

            auto dist2 = static_cast<float>(dx * dx + dz * dz);
            float falloff = 1.0f / (1.0f + dist2); // 0.2..1

            float tToWrite = ambientTemperatureK +
                             (baseVisualT - ambientTemperatureK) * falloff;

            float &cellT = m_grid.m_cells[nz * W + nx];

            if (cellT < 0.0f) {
              continue;
            }

            if (isBurningPoint) {
              if (tToWrite > cellT) {
                cellT = tToWrite;
              }
            } else {
              if (std::abs(cellT - ambientTemperatureK) < AMBIENT_EPS_K &&
                  tToWrite < cellT) {
                cellT = tToWrite;
              }
            }
          }
        }
      }
    }

    for (float T : m_grid.m_cells) {
      if (T < 0.0F) // burned
        continue;
      if (T < minCellT) {
        minCellT = T;
      }
      if (T > maxCellT) {
        maxCellT = T;
      }
    }

    if (minCellT == std::numeric_limits<float>::max()) {
      minCellT = maxCellT = ambientTemperatureK;
    }

    m_stats.m_ambientT = ambientTemperatureK;
    m_stats.m_minCellT = minCellT;
    m_stats.m_maxCellT = maxCellT;
    m_stats.m_cooledPoints = cooledPoints;
    m_stats.m_burnedAgents = burnedAgents;
  }

  void UploadToTexture() {
    if (!m_texture) {
      return;
    }

    const int W = m_grid.m_width;
    const int H = m_grid.m_height;
    const int N = W * H;
    if (W <= 0 || H <= 0 || m_grid.m_cells.size() != static_cast<size_t>(N)) {
      return;
    }

    std::vector<unsigned char> pixels(static_cast<size_t>(N) * 4U);

    const float AMBIENT_C = m_stats.m_ambientT - 273.15F;

    constexpr float COLD_RANGE_C = 20.0F;
    constexpr float WARM_RANGE_C = 60.0F;
    constexpr float FIRE_RANGE_C = 800.0F;
    for (int i = 0; i < N; ++i) {
      float T = m_grid.m_cells[i];
      glm::vec3 color;

      if (T < 0.0F) {
        // выгоревшие – тёмно-серые
        color = glm::vec3(0.15F, 0.15F, 0.15F);
      } else {
        float Tc = T - 273.15F;
        float deltaC = Tc - AMBIENT_C;
        float t01 = 0.5F; // центр по умолчанию

        if (deltaC <= -COLD_RANGE_C) {
          t01 = 0.0f;
        } else if (deltaC < 0.0F) {
          float k = (deltaC + COLD_RANGE_C) / COLD_RANGE_C; // 0..1
          t01 = 0.25F * k;
        } else if (deltaC < WARM_RANGE_C) {
          float k = deltaC / WARM_RANGE_C; // 0..1
          t01 = 0.25F + (0.25F * k);
        } else {
          float fireDelta =
              glm::clamp(deltaC - WARM_RANGE_C, 0.0F, FIRE_RANGE_C);
          float k = fireDelta / FIRE_RANGE_C;
          k = std::sqrt(k);
          t01 = 0.5F + (0.5F * k);
        }

        color = ColorFromHeat01(t01);
      }

      pixels[(4 * i) + 0] =
          static_cast<unsigned char>(glm::clamp(color.r, 0.0F, 1.0F) * 255.0F);
      pixels[(4 * i) + 1] =
          static_cast<unsigned char>(glm::clamp(color.g, 0.0F, 1.0F) * 255.0F);
      pixels[(4 * i) + 2] =
          static_cast<unsigned char>(glm::clamp(color.b, 0.0F, 1.0F) * 255.0F);
      pixels[(4 * i) + 3] =
          static_cast<unsigned char>(0.8F * 255.0F); // прозрачноcть оверлея
    }

    m_texture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));
  }
};

} // namespace engine
