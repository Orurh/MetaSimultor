#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "core/agent.h"
#include "graphics/Texture.h"

namespace engine {

class HeatMap {
  public:
  struct Grid {
    int width = 0;
    int height = 0;
    float worldMinX = 0.0f;
    float worldMaxX = 0.0f;
    float worldMinZ = 0.0f;
    float worldMaxZ = 0.0f;
    std::vector<float> cells;

    void Resize(int w, int h) {
      width = w;
      height = h;
      cells.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0f);
    }
  };

  struct Stats {
    float minT = 0.0f;
    float maxT = 0.0f;
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

  // Настройка world-границ, в которые будем вписывать XZ-координаты
  void SetWorldBounds(float minX, float maxX, float minZ, float maxZ) {
    m_grid.worldMinX = minX;
    m_grid.worldMaxX = maxX;
    m_grid.worldMinZ = minZ;
    m_grid.worldMaxZ = maxZ;
  }

  void UpdateFromAgents(const std::vector<Agent> &agents, float ambientTemperature) {
    if (!m_texture) {
      Init(256, 256);
    }
    BuildGridFromAgents(agents, ambientTemperature);
    UploadToTexture();
  }

  std::shared_ptr<Texture> GetTexture() const { return m_texture; }
  Stats GetStats() const { return m_stats; }
  glm::vec2 GetWorldMinXZ() const { return {m_grid.worldMinX, m_grid.worldMinZ}; }
  glm::vec2 GetWorldMaxXZ() const { return {m_grid.worldMaxX, m_grid.worldMaxZ}; }
  const Grid &GetGrid() const { return m_grid; }

  private:
  Grid m_grid;
  Stats m_stats;
  std::shared_ptr<Texture> m_texture;

  bool m_useGPU = false;
  bool m_gpuInitialized = false;

  static constexpr float MIN_TEMPERATURE = 0.0f;
  static constexpr float MAX_TEMPERATURE = 1560.0f;
  static int WorldToGrid(float x, float worldMin, float worldMax, int size) {
    const float len = worldMax - worldMin;
    if (len <= 1e-5f) {
      return 0;
    }
    float t = (x - worldMin) / len;
    int i = static_cast<int>(t * static_cast<float>(size));
    if (i < 0)
      i = 0;
    if (i >= size)
      i = size - 1;
    return i;
  }
  void BuildGridFromAgents(const std::vector<Agent> &agents, float ambientTemperature) {
    const int W = m_grid.width;
    const int H = m_grid.height;

    if (W <= 0 || H <= 0) {
      return;
    }

    m_grid.cells.assign(static_cast<size_t>(W) * H, ambientTemperature);

    int totalPoints = 0;
    int cooledPoints = 0;
    int burnedAgents = 0;

    for (const Agent &a : agents) {
      const FireCollider &c = a.fire_collider;
      const int N = c.point_count;
      if (N <= 0)
        continue;
      // Проверяем сгорел ли агент
      bool isBurned = (a.type == FireAgentState::Burned);
      if (isBurned)
        burnedAgents++;
      for (int i = 0; i < N; ++i) {
        totalPoints++;
        const Float3 &p = c.heat_points[i];
        float T = c.temperatures[i];
        float moisture = c.moisture[i];

        float coolingEffect = moisture * moisture * 50.0f;
        float adjustedT = T - coolingEffect;

        if (coolingEffect > 1.0f)
          cooledPoints++;

        int gx = WorldToGrid(p.x, m_grid.worldMinX, m_grid.worldMaxX, W);
        int gz = WorldToGrid(p.z, m_grid.worldMinZ, m_grid.worldMaxZ, H);

        const int idx = gz * W + gx;

        if (isBurned) {
          m_grid.cells[idx] = -100.0f;
        } else {
          m_grid.cells[idx] += (adjustedT - ambientTemperature) * 0.1f;
        }
      }
    }

    m_stats.minT = MIN_TEMPERATURE;
    m_stats.maxT = MAX_TEMPERATURE;
  }
  static glm::vec3 ColorFrom01(float t) {
    if (t < -0.1f) {
      return glm::vec3(0.2, 0.2f, 0.2f);
    }

    t = glm::clamp(t, 0.0f, 1.0f);

    float hue = 240.0f * (1.0f - t);
    float saturation = 1.0f;
    float lightness = 0.5f;

    return HSLtoRGB(hue, saturation, lightness);
  }

  static glm::vec3 HSLtoRGB(float h, float s, float l) {
    h = glm::mod(h, 360.0f);
    s = glm::clamp(s, 0.0f, 1.0f);
    l = glm::clamp(l, 0.0f, 1.0f);

    float c = (1.0f - glm::abs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - glm::abs(glm::mod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    glm::vec3 rgb;
    if (h < 60.0f) {
      rgb = glm::vec3(c, x, 0.0f);
    } else if (h < 120.0f) {
      rgb = glm::vec3(x, c, 0.0f);
    } else if (h < 180.0f) {
      rgb = glm::vec3(0.0f, c, x);
    } else if (h < 240.0f) {
      rgb = glm::vec3(0.0f, x, c);
    } else if (h < 300.0f) {
      rgb = glm::vec3(x, 0.0f, c);
    } else {
      rgb = glm::vec3(c, 0.0f, x);
    }

    return rgb + glm::vec3(m);
  }
  void UploadToTexture() {
    if (!m_texture)
      return;

    const int W = m_grid.width;
    const int H = m_grid.height;
    const int N = W * H;

    if (W <= 0 || H <= 0 || m_grid.cells.size() != static_cast<size_t>(N))
      return;

    std::vector<unsigned char> pixels;
    pixels.resize(static_cast<size_t>(N) * 4u);

    for (int i = 0; i < N; ++i) {
      float T = m_grid.cells[i];

      glm::vec3 c;

      if (T < -50.0f) {
        c = glm::vec3(0.2f, 0.2f, 0.2f);
      } else {
        // Нормализуем обычные температуры
        float t01 = (T - MIN_TEMPERATURE) / (MAX_TEMPERATURE - MIN_TEMPERATURE);
        t01 = glm::clamp(t01, 0.0f, 1.0f);
        c = ColorFrom01(t01);
      }

      pixels[4 * i + 0] = static_cast<unsigned char>(c.r * 255.0f);
      pixels[4 * i + 1] = static_cast<unsigned char>(c.g * 255.0f);
      pixels[4 * i + 2] = static_cast<unsigned char>(c.b * 255.0f);
      pixels[4 * i + 3] = static_cast<unsigned char>(0.6f * 255.0f);
    }

    m_texture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));
  }
};

}  // namespace engine