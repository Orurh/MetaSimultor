#include "Terrain.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include <glm/gtc/noise.hpp>
#include <random>
#include "graphics/IndexBuffer.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"
namespace engine {
class Terrain::PImpl {
 public:
  int m_width = 0;
  int m_depth = 0;
  float m_scale = 1.0f;
  float m_maxHeight = 100.0f;
  uint32_t m_seed = 0;
  float m_frequency = 0.02f;
  int m_octaves = 6;
  float m_persistence = 0.5f;
  float m_lacunarity = 2.0f;
  glm::vec3 m_position = glm::vec3(0.0f);
  glm::vec2 m_noiseOffset{0.0f};
  std::vector<std::vector<float>> m_heightMap;
  std::shared_ptr<VertexArrayObject> m_vao;
  [[nodiscard]] bool empty() const noexcept {
    return m_width <= 0 || m_depth <= 0 || m_heightMap.empty();
  }

  [[nodiscard]] float HeightAtNearest(int ix, int iz) const {
    if (empty()) {
      return 0.0F;
    }
    ix = std::clamp(ix, 0, m_width - 1);
    iz = std::clamp(iz, 0, m_depth - 1);
    return m_heightMap[ix][iz];
  }

  [[nodiscard]] float SampleHeightBilinear(float x, float z) const {
    if (empty()) {
      return 0.0F;
    }
    // x,z — в ЛОКАЛЬНЫХ координатах heightMap (1 texel = 1)
    const int x0 = static_cast<int>(std::floor(x));
    const int z0 = static_cast<int>(std::floor(z));
    const int x1 = x0 + 1;
    const int z1 = z0 + 1;
    const float tx = std::clamp(x - x0, 0.0f, 1.0f);  // ← клампы
    const float tz = std::clamp(z - z0, 0.0f, 1.0f);

    const float h00 = HeightAtNearest(x0, z0);
    const float h10 = HeightAtNearest(x1, z0);
    const float h01 = HeightAtNearest(x0, z1);
    const float h11 = HeightAtNearest(x1, z1);

    const float h0 = h00 * (1.0f - tx) + h10 * tx;
    const float h1 = h01 * (1.0f - tx) + h11 * tx;
    return h0 * (1.0f - tz) + h1 * tz;
  }
  void GenerateHeightmapRandom(const TerrainGenerationSettings& s) {
    m_width = s.width;
    m_depth = s.depth;
    m_scale = s.scale;
    m_maxHeight = s.maxHeight;
    m_seed = s.seed;
    m_frequency = s.frequency;  
    m_octaves = s.octaves;
    m_persistence = s.persistence;
    m_lacunarity = s.lacunarity;

    // Случайное смещение шума (в МЕТРАХ), чтобы паттерн не "прилипал" к (0,0)
    std::mt19937 gen(m_seed);
    std::uniform_real_distribution<float> dist(0.0f, 10000.0f);
    m_noiseOffset = {dist(gen), dist(gen)};

    // Высотная карта: [x][z]
    m_heightMap.assign(m_width, std::vector<float>(m_depth, 0.0f));

    // Заполняем: p = (x,z) в ТЕКСЕЛЯХ → внутри FBM переводим в МЕТРЫ
    for (int z = 0; z < m_depth; ++z) {
      for (int x = 0; x < m_width; ++x) {
        const float h01 = (FBM(glm::vec2{float(x), float(z)}, s) + 1.0f) * 0.5f;  // [-1..1]→[0..1]
        m_heightMap[x][z] = h01 * m_maxHeight;
      }
    }
  }

  // --- FBM: p — в ТЕКСЕЛЯХ, конвертируем в метры перед семплингом ---
  [[nodiscard]] float FBM(glm::vec2 p, const TerrainGenerationSettings& s) const {
    // переводим в метры, чтобы frequency читалась как циклы/метр
    const glm::vec2 pMeters = p * m_scale;

    float total = 0.0f;
    float amplitude = 1.0f;
    float freq = s.frequency;  // циклы на метр
    float maxValue = 0.0f;

    for (int i = 0; i < s.octaves; ++i) {
      const glm::vec2 coord = (pMeters + m_noiseOffset) * freq;
      const float n = glm::perlin(coord);  // ∈ [-1..+1]
      total += n * amplitude;
      maxValue += amplitude;

      amplitude *= s.persistence;
      freq *= s.lacunarity;
    }

    return (maxValue > 0.0f) ? (total / maxValue) : 0.0f;  // ∈ [-1..+1]
  }

  // --- Построение меша с устойчивыми нормалями/UV и reserve() ---
  void GenerateMesh() {
    // Пусто — очищаем VAO и выходим
    if (m_width <= 0 || m_depth <= 0 || m_heightMap.empty()) {
      m_vao.reset();
      return;
    }

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    // pos(3) + nrm(3) + uv(2)
    vertices.reserve(static_cast<size_t>(m_width) * static_cast<size_t>(m_depth) * 8);
    indices.reserve(static_cast<size_t>(m_width - 1) * static_cast<size_t>(m_depth - 1) * 6);

    // Удобный семплер (билинейный, безопасный по границам)
    auto H = [&](float x, float z) -> float {
      return SampleHeightBilinear(x, z);
    };

    // Шаг для производных в ЛОКАЛИ (1 тексель); в МИРЕ это eps * m_scale
    constexpr float eps = 1.0f;

    // Вершины
    for (int z = 0; z < m_depth; ++z) {
      for (int x = 0; x < m_width; ++x) {
        const float worldX = x * m_scale;
        const float worldZ = z * m_scale;
        const float y = m_heightMap[x][z];

        // Центральные разности в ЛОКАЛЬНЫХ координатах с учётом масштаба по XZ
        const float hx0 = H(float(x) - eps, float(z));
        const float hx1 = H(float(x) + eps, float(z));
        const float hz0 = H(float(x), float(z) - eps);
        const float hz1 = H(float(x), float(z) + eps);

        const float denom = 2.0f * eps * std::max(1e-6f, m_scale);
        const float dHdX = (hx1 - hx0) / denom;
        const float dHdZ = (hz1 - hz0) / denom;

        const glm::vec3 nrm = glm::normalize(glm::vec3(-dHdX, 1.0f, -dHdZ));

        const float u = (m_width > 1) ? float(x) / float(m_width - 1) : 0.0f;
        const float v = (m_depth > 1) ? float(z) / float(m_depth - 1) : 0.0f;

        vertices.insert(vertices.end(), {worldX, y, worldZ, nrm.x, nrm.y, nrm.z, u, v});
      }
    }

    // Индексы (два треугольника на quad)
    for (int z = 0; z < m_depth - 1; ++z) {
      for (int x = 0; x < m_width - 1; ++x) {
        const uint32_t i = static_cast<uint32_t>(x + z * m_width);
        const uint32_t iR = static_cast<uint32_t>(x + 1 + z * m_width);
        const uint32_t iD = static_cast<uint32_t>(x + (z + 1) * m_width);
        const uint32_t iDR = static_cast<uint32_t>(x + 1 + (z + 1) * m_width);

        indices.insert(indices.end(), {i, iD, iR, iR, iD, iDR});
      }
    }

    // GPU буферы
    auto vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
    vbo->SetLayout({{ShaderDataType::Float3, "a_Position"},
                    {ShaderDataType::Float3, "a_Normal"},
                    {ShaderDataType::Float2, "a_TexCoord"}});

    auto ibo = IndexBuffer::Create(indices.data(), indices.size());

    m_vao = VertexArrayObject::Create();
    m_vao->AddVertexBuffer(vbo);
    m_vao->SetIndexBuffer(ibo);
  }
};

Terrain::Terrain() : m_impl(std::make_unique<PImpl>()) {}
Terrain::~Terrain() = default;

void Terrain::GenerateRandom(const TerrainGenerationSettings& s) {
  glm::vec3 keepPos = m_impl->m_position;  // сохранить текущее смещение
  m_impl->GenerateHeightmapRandom(s);
  m_impl->GenerateMesh();
  m_impl->m_position = keepPos;  // вернуть
}

int Terrain::GetWidth() const {
  return m_impl->m_width;
}
int Terrain::GetDepth() const {
  return m_impl->m_depth;
}

float Terrain::GetHeightAt(float worldX, float worldZ) const {
  const glm::vec3 pos = m_impl->m_position;
  const float s = std::max(1e-6f, m_impl->m_scale);
  const float lx = (worldX - pos.x) / s;
  const float lz = (worldZ - pos.z) / s;

  if (m_impl->empty())
    return pos.y;
  if (lx < 0.f || lz < 0.f || lx > float(m_impl->m_width - 1) || lz > float(m_impl->m_depth - 1))
    return pos.y;  // вместо «плато» на границе

  const float hLocal = m_impl->SampleHeightBilinear(lx, lz);
  return pos.y + hLocal;
}

glm::vec3 Terrain::GetNormalAt(float worldX, float worldZ) const {
  const glm::vec3 pos = m_impl->m_position;
  const float s = std::max(1e-6f, m_impl->m_scale);

  const float lx = (worldX - pos.x) / s;
  const float lz = (worldZ - pos.z) / s;

  auto H = [&](float x, float z) {
    return m_impl->SampleHeightBilinear(x, z);
  };

  const float eps = 1.0f;  // 1 texel в ЛОКАЛИ
  const float hx0 = H(lx - eps, lz);
  const float hx1 = H(lx + eps, lz);
  const float hz0 = H(lx, lz - eps);
  const float hz1 = H(lx, lz + eps);

  // dx,dz в МИРЕ: eps * s
  const float dHdX = (hx1 - hx0) / (2.0f * eps * s);
  const float dHdZ = (hz1 - hz0) / (2.0f * eps * s);

  return glm::normalize(glm::vec3(-dHdX, 1.0f, -dHdZ));
}

std::shared_ptr<VertexArrayObject> Terrain::GetVAO() const {
  return m_impl->m_vao;
}
float Terrain::GetScale() const {
  return m_impl->m_scale;
}
float Terrain::GetMaxHeight() const {
  return m_impl->m_maxHeight;
}
uint32_t Terrain::GetSeed() const {
  return m_impl->m_seed;
}
float Terrain::GetFrequency() const {
  return m_impl->m_frequency;
}
int Terrain::GetOctaves() const {
  return m_impl->m_octaves;
}
float Terrain::GetPersistence() const {
  return m_impl->m_persistence;
}
float Terrain::GetLacunarity() const {
  return m_impl->m_lacunarity;
}
glm::vec3 Terrain::GetPosition() const {
  return m_impl->m_position;
}
void Terrain::SetPosition(const glm::vec3& pos) {
  m_impl->m_position = pos;
}

}  // namespace engine