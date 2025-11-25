#include "heatMap.h"

#include "risk_predictor.h" // путь подправь под свой

using namespace engine;

void HeatMap::Init(int width, int height) {
  m_grid.Resize(width, height);

  TextureSpecification spec;
  spec.Width = static_cast<uint32_t>(width);
  spec.Height = static_cast<uint32_t>(height);
  spec.Format = ImageFormat::RGBA8;
  spec.GenerateMips = false;

  m_texture = Texture::Create(spec);
}

void HeatMap::SetWorldBounds(float minX, float maxX, float minZ,
                             float maxZ) {
  m_grid.m_worldMinX = minX;
  m_grid.m_worldMaxX = maxX;
  m_grid.m_worldMinZ = minZ;
  m_grid.m_worldMaxZ = maxZ;
}

void HeatMap::UpdateFromAgents(const std::vector<Agent> &agents,
                               float ambientTemperatureK) {
  if (!m_texture) {
    Init(256, 256);
  }

  // Температурная карта — всегда строим по текущим агентам
  BuildGridFromAgents(agents, ambientTemperatureK);

  // Карта риска:
  //  - в режиме Temperature — полностью очищаем (ничего не показываем)
  //  - в режиме Risk       — НЕ трогаем, сюда заранее залил RiskPredictor
  if (m_mode != HeatMapMode::Risk) {
    m_grid.m_risk.assign(
        static_cast<size_t>(m_grid.m_width) *
        static_cast<size_t>(m_grid.m_height),
        0.0F);
  }

  UploadToTexture();
}

int HeatMap::WorldToGrid(float x, float worldMin, float worldMax,
                         int size) {
  const float len = worldMax - worldMin;
  if (len <= 1e-5F)
    return 0;

  float t = (x - worldMin) / len;
  int i = static_cast<int>(t * static_cast<float>(size));
  if (i < 0)
    i = 0;
  if (i >= size)
    i = size - 1;
  return i;
}

glm::vec3 HeatMap::ColorFromHeat01(float t) {
  t = glm::clamp(t, 0.0F, 1.0F);

  if (t < 0.25F) {
    float k = t / 0.25F;
    return glm::mix(glm::vec3(0.0F, 0.0F, 0.4F),
                    glm::vec3(0.0F, 0.8F, 1.0F),
                    k); // dark blue → cyan
  }
  if (t < 0.5F) {
    float k = (t - 0.25F) / 0.25F;
    return glm::mix(glm::vec3(0.0F, 0.8F, 1.0F),
                    glm::vec3(0.0F, 1.0F, 0.0F),
                    k); // cyan → green
  }
  if (t < 0.75F) {
    float k = (t - 0.5F) / 0.25F;
    return glm::mix(glm::vec3(0.0F, 1.0F, 0.0F),
                    glm::vec3(1.0F, 0.8F, 0.0F),
                    k); // green → yellow/orange
  }
  float k = (t - 0.75F) / 0.25F;
  return glm::mix(glm::vec3(1.0F, 0.8F, 0.0F),
                  glm::vec3(1.0F, 0.1F, 0.0F),
                  k); // orange → red
}

glm::vec3 HeatMap::ColorFromRisk01(float r) {
  r = glm::clamp(r, 0.0F, 1.0F);

  if (r < 0.05F) {
    return glm::vec3(0.05F, 0.05F, 0.08F);
  }
  if (r < 0.33F) {
    float k = (r - 0.05F) / (0.33F - 0.05F);
    return glm::mix(glm::vec3(0.0F, 0.2F, 0.0F),
                    glm::vec3(0.0F, 0.6F, 0.0F),
                    k); // тёмно-зелёный → зелёный
  }
  if (r < 0.66F) {
    float k = (r - 0.33F) / (0.66F - 0.33F);
    return glm::mix(glm::vec3(0.0F, 0.6F, 0.0F),
                    glm::vec3(0.9F, 0.8F, 0.0F),
                    k); // зелёный → жёлтый
  }
  if (r < 0.9F) {
    float k = (r - 0.66F) / (0.9F - 0.66F);
    return glm::mix(glm::vec3(0.9F, 0.8F, 0.0F),
                    glm::vec3(1.0F, 0.5F, 0.0F),
                    k); // жёлтый → оранжевый
  }
  float k = (r - 0.9F) / 0.1F;
  return glm::mix(glm::vec3(1.0F, 0.5F, 0.0F),
                  glm::vec3(1.0F, 0.0F, 0.0F),
                  k); // оранжевый → красный
}

void HeatMap::BuildGridFromAgents(const std::vector<Agent> &agents,
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

  // ---------- заполнение температур по агентам ----------
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

        if (coolingC > 0.5F)
          ++cooledPoints;
      }

      // Размазываем влияние по соседним ячейкам
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

          float dist2 = static_cast<float>(dx * dx + dz * dz);
          float falloff = 1.0F / (1.0F + dist2); // 0.2..1

          float tToWrite = ambientTemperatureK +
                           (baseVisualT - ambientTemperatureK) * falloff;

          float &cellT = m_grid.m_cells[nz * W + nx];

          if (cellT < 0.0F) {
            continue; // burned
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

  // ---------- финальный проход: min/max + "температурный риск" ----------

  const float AMBIENT_K = ambientTemperatureK;
  const float AMBIENT_C2 = AMBIENT_K - 273.15F; // другое имя, чтобы не путаться

  // пороги для "тёплых" и "критических" зон (°C выше среды)
  constexpr float WARM_THR_C = 20.0F;      // уже заметно нагрелось
  constexpr float CRITICAL_THR_C = 150.0F; // сильно горячо, высокий риск

  float sumDeltaC = 0.0F;
  float maxDeltaC = std::numeric_limits<float>::lowest();
  int cellCount = 0;
  int warmCells = 0;
  int criticalCells = 0;

  for (float T : m_grid.m_cells) {
    if (T < 0.0F) // burned
      continue;

    if (T < minCellT)
      minCellT = T;
    if (T > maxCellT)
      maxCellT = T;

    float Tc = T - 273.15F;
    float deltaC = Tc - AMBIENT_C2;

    sumDeltaC += deltaC;
    if (deltaC > maxDeltaC)
      maxDeltaC = deltaC;

    if (deltaC > WARM_THR_C)
      ++warmCells;
    if (deltaC > CRITICAL_THR_C)
      ++criticalCells;

    ++cellCount;
  }

  if (minCellT == std::numeric_limits<float>::max()) {
    minCellT = maxCellT = ambientTemperatureK;
  }

  // заполняем stats
  m_stats.m_ambientT = ambientTemperatureK;
  m_stats.m_minCellT = minCellT;
  m_stats.m_maxCellT = maxCellT;
  m_stats.m_cooledPoints = cooledPoints;
  m_stats.m_burnedAgents = burnedAgents;

  if (cellCount > 0) {
    m_stats.m_meanDeltaC = sumDeltaC / static_cast<float>(cellCount);
    m_stats.m_maxDeltaC = maxDeltaC;
    m_stats.m_warmArea01 = static_cast<float>(warmCells) / cellCount;
    m_stats.m_criticalArea01 = static_cast<float>(criticalCells) / cellCount;
  } else {
    m_stats.m_meanDeltaC = 0.0F;
    m_stats.m_maxDeltaC = 0.0F;
    m_stats.m_warmArea01 = 0.0F;
    m_stats.m_criticalArea01 = 0.0F;
  }
}

void HeatMap::UpdateRiskPrediction(const RiskPredictor& predictor) {
  if (!m_texture) {
    Init(256, 256);
  }

  const int W = m_grid.m_width;
  const int H = m_grid.m_height;
  if (W <= 0 || H <= 0) {
    return;
  }

  const int N = W * H;
  m_grid.m_risk.assign(static_cast<size_t>(N), 0.0f);

  // Берём всех risk-агентов сразу
  const auto& rAgents = predictor.GetRiskAgents();
  if (rAgents.empty())
    return;

  // Радиус в ячейках, вокруг которого размазываем риск
  constexpr int STAMP_RADIUS = 3;

  for (const auto& ra : rAgents) {
    if (!ra.will_burn)
      continue;

    // базовый риск от агента: можно потом модифицировать через burn_time
    float baseRisk = 1.0f;

    // позиция агента в координатах сетки
    int gx = WorldToGrid(ra.position.x, m_grid.m_worldMinX, m_grid.m_worldMaxX, W);
    int gz = WorldToGrid(ra.position.z, m_grid.m_worldMinZ, m_grid.m_worldMaxZ, H);
    if (gx < 0 || gx >= W || gz < 0 || gz >= H)
      continue;

    for (int dz = -STAMP_RADIUS; dz <= STAMP_RADIUS; ++dz) {
      int nz = gz + dz;
      if (nz < 0 || nz >= H)
        continue;

      for (int dx = -STAMP_RADIUS; dx <= STAMP_RADIUS; ++dx) {
        int nx = gx + dx;
        if (nx < 0 || nx >= W)
          continue;

        float dist2 = static_cast<float>(dx * dx + dz * dz);
        float falloff = 1.0f / (1.0f + dist2); // 1 в центре, меньше по краям

        float r = baseRisk * falloff;
        float& cellRisk = m_grid.m_risk[nz * W + nx];

        // берём максимум: если несколько источников, побеждает больший риск
        if (r > cellRisk)
          cellRisk = r;
      }
    }
  }

  // если сейчас включён режим Risk — сразу обновляем текстуру
  if (m_mode == HeatMapMode::Risk) {
    UploadToTexture();
  }
}


void HeatMap::UploadToTexture() {
  if (!m_texture)
    return;

  switch (m_mode) {
    case HeatMapMode::Temperature:
      UploadTemperatureToTexture();
      break;
    case HeatMapMode::Risk:
      UploadRiskToTexture();
      break;
  }
}

void HeatMap::UploadTemperatureToTexture() {
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
        t01 = 0.0F;
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
        static_cast<unsigned char>(0.8F * 255.0F); // прозрачность оверлея
  }

  m_texture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));
}

void HeatMap::UploadRiskToTexture() {
  const int W = m_grid.m_width;
  const int H = m_grid.m_height;
  const int N = W * H;
  if (W <= 0 || H <= 0 || m_grid.m_risk.size() != static_cast<size_t>(N)) {
    return;
  }

  std::vector<unsigned char> pixels(static_cast<size_t>(N) * 4U);

  for (int i = 0; i < N; ++i) {
    float risk = m_grid.m_risk[i];
    glm::vec3 color = ColorFromRisk01(risk);

    pixels[(4 * i) + 0] =
        static_cast<unsigned char>(glm::clamp(color.r, 0.0F, 1.0F) * 255.0F);
    pixels[(4 * i) + 1] =
        static_cast<unsigned char>(glm::clamp(color.g, 0.0F, 1.0F) * 255.0F);
    pixels[(4 * i) + 2] =
        static_cast<unsigned char>(glm::clamp(color.b, 0.0F, 1.0F) * 255.0F);
    pixels[(4 * i) + 3] =
        static_cast<unsigned char>(0.8F * 255.0F); // прозрачность оверлея
  }

  m_texture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));
}

void HeatMap::Grid::Resize(int w, int h) {
  m_width = w;
  m_height = h;
  m_cells.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0F);
  m_risk.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0F);
}
