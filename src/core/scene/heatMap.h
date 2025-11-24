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
    int width = 0;
    int height = 0;

    float worldMinX = 0.0f;
    float worldMaxX = 0.0f;
    float worldMinZ = 0.0f;
    float worldMaxZ = 0.0f;

    std::vector<float> cells;  // size = width * height

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

  // Инициализация: создаём grid и текстуру нужного размера
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

  // Пересобрать карту температур из агентов и залить в текстуру
  void UpdateFromAgents(const std::vector<Agent>& agents, float ambientTemperature) {
    if (!m_texture) {
      // если вдруг Init не вызывали — создадим что-нибудь по умолчанию
      Init(256, 256);
    }

    BuildGridFromAgents(agents, ambientTemperature);
    UploadToTexture();
  }

  // Доступ к текстуре для рендера
  std::shared_ptr<Texture> GetTexture() const { return m_texture; }

  // Статистика по температуре
  Stats GetStats() const { return m_stats; }

  // Границы в мире по XZ
  glm::vec2 GetWorldMinXZ() const { return {m_grid.worldMinX, m_grid.worldMinZ}; }

  glm::vec2 GetWorldMaxXZ() const { return {m_grid.worldMaxX, m_grid.worldMaxZ}; }

  const Grid& GetGrid() const { return m_grid; }

 private:
  Grid m_grid;
  Stats m_stats;
  std::shared_ptr<Texture> m_texture;

 private:
  // [worldMin, worldMax] -> [0, size)
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

  // Простейший градиент: синий -> зелёный -> красный
  static glm::vec3 ColorFrom01(float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    if (t < 0.5f) {
      // 0..0.5: синий -> зелёный
      float k = t / 0.5f;
      return glm::vec3(0.0f, k, 1.0f - k);
    } else {
      // 0.5..1: зелёный -> красный
      float k = (t - 0.5f) / 0.5f;
      return glm::vec3(k, 1.0f - k, 0.0f);
    }
  }

  void BuildGridFromAgents(const std::vector<Agent>& agents, float ambientTemperature) {
    const int W = m_grid.width;
    const int H = m_grid.height;

    if (W <= 0 || H <= 0) {
      return;
    }

    m_grid.cells.assign(static_cast<size_t>(W) * H, ambientTemperature);

    float minT = std::numeric_limits<float>::max();
    float maxT = std::numeric_limits<float>::lowest();

    for (const Agent& a : agents) {
      const FireCollider& c = a.fire_collider;
      const int N = c.point_count;
      if (N <= 0)
        continue;

      for (int i = 0; i < N; ++i) {
        const Float3& p = c.heat_points[i];
        float T = c.temperatures[i];

        int gx = WorldToGrid(p.x, m_grid.worldMinX, m_grid.worldMaxX, W);
        int gz = WorldToGrid(p.z, m_grid.worldMinZ, m_grid.worldMaxZ, H);

        const int idx = gz * W + gx;
        float& cellT = m_grid.cells[idx];

        if (T > cellT) {
          cellT = T;
        }
      }
    }

    // Собираем min/max
    for (float T : m_grid.cells) {
      if (T < minT)
        minT = T;
      if (T > maxT)
        maxT = T;
    }

    if (minT == std::numeric_limits<float>::max()) {
      // Ни одной точки не было – считаем, что всё на уровне ambient
      minT = maxT = ambientTemperature;
    }

    m_stats.minT = minT;
    m_stats.maxT = maxT;
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

    const float Tmin = m_stats.minT;
    const float Tmax = m_stats.maxT;
    const float eps = 1e-3f;

    for (int i = 0; i < N; ++i) {
      float T = m_grid.cells[i];

      float t01 = 0.0f;
      if ((Tmax - Tmin) > eps) {
        t01 = (T - Tmin) / (Tmax - Tmin);
      }

      glm::vec3 c = ColorFrom01(t01);

      pixels[4 * i + 0] = static_cast<unsigned char>(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f);
      pixels[4 * i + 1] = static_cast<unsigned char>(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f);
      pixels[4 * i + 2] = static_cast<unsigned char>(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f);

      // Альфа – постоянная (чуть прозрачная)
      pixels[4 * i + 3] = static_cast<unsigned char>(0.6f * 255.0f);
    }

    // Используем новый метод SetData
    m_texture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));
  }
};

}  // namespace engine