#include "Terrain.h"
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "graphics/IndexBuffer.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"
#include <glm/gtc/noise.hpp>
#include <random>
namespace engine {
class Terrain::PImpl {
public:
  int m_width;
  int m_depth;
  float m_scale = 1.0f;
  float m_maxHeight = 100.0f;
  uint32_t m_seed = 0;
  float m_frequency = 0.02f;
  int m_octaves = 6;
  float m_persistence = 0.5f;
  float m_lacunarity = 2.0f;
  glm::vec3 m_position = glm::vec3(0.0f);
  std::vector<std::vector<float>> m_heightMap;
  std::shared_ptr<VertexArrayObject> m_vao;

  float FBM(glm::vec2 p, const TerrainGenerationSettings &s) const {
    std::mt19937 gen(s.seed);
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);
    glm::vec2 offset(dist(gen), dist(gen));

    float total = 0.0f;
    float amplitude = 1.0f;
    float freq = s.frequency;
    float maxValue = 0.0f;

    for (int i = 0; i < s.octaves; ++i) {
      glm::vec2 coord = (p + offset) * freq;
      total += glm::perlin(coord) * amplitude;
      maxValue += amplitude;
      amplitude *= s.persistence;
      freq *= s.lacunarity;
    }

    return total / maxValue;
  }

  void GenerateHeightmapRandom(const TerrainGenerationSettings &s) {
    m_width = s.width;
    m_depth = s.depth;
    m_scale = s.scale;
    m_maxHeight = s.maxHeight;
    m_seed = s.seed;
    m_frequency = s.frequency;
    m_octaves = s.octaves;
    m_persistence = s.persistence;
    m_lacunarity = s.lacunarity;
    m_heightMap.assign(m_width, std::vector<float>(m_depth, 0.0f));

    for (int z = 0; z < m_depth; ++z) {
      for (int x = 0; x < m_width; ++x) {
        float nx = static_cast<float>(x);
        float nz = static_cast<float>(z);

        float h = FBM({nx, nz}, s);

        h = (h + 1.0f) * 0.5f;

        m_heightMap[x][z] = h * s.maxHeight;
      }
    }
  }
  // В PImpl::GenerateMesh()
  void GenerateMesh() {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    for (int z = 0; z < m_depth; ++z) {
      for (int x = 0; x < m_width; ++x) {
        float worldX = x * m_scale;
        float worldZ = z * m_scale;
        float y = m_heightMap[x][z];

        glm::vec3 normal = {0, 1, 0};
        if (x > 0 && x < m_width - 1 && z > 0 && z < m_depth - 1) {
          float hL = m_heightMap[x - 1][z];
          float hR = m_heightMap[x + 1][z];
          float hD = m_heightMap[x][z - 1];
          float hU = m_heightMap[x][z + 1];
          normal = glm::normalize(glm::vec3(hL - hR, 2.0f * m_scale, hD - hU));
        }

        // UV
        float u = static_cast<float>(x) / (m_width - 1);
        float v = static_cast<float>(z) / (m_depth - 1);

        vertices.insert(vertices.end(), {worldX, y, worldZ, normal.x, normal.y,
                                         normal.z, u, v});
      }
    }

    for (int z = 0; z < m_depth - 1; ++z) {
      for (int x = 0; x < m_width - 1; ++x) {
        uint32_t i = x + z * m_width;
        indices.insert(indices.end(), {i, i + m_width, i + 1, i + 1,
                                       i + m_width, i + m_width + 1});
      }
    }

    auto vbo =
        VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
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

void Terrain::GenerateRandom(const TerrainGenerationSettings &s) {
  m_impl->GenerateHeightmapRandom(s);
  m_impl->GenerateMesh();
}

int Terrain::GetWidth() const { return m_impl->m_width; }
int Terrain::GetDepth() const { return m_impl->m_depth; }
float Terrain::GetHeightAt(float worldX, float worldZ) const {
  int x = static_cast<int>(worldX / m_impl->m_scale);
  int z = static_cast<int>(worldZ / m_impl->m_scale);

  if (x < 0 || x >= m_impl->m_width || z < 0 || z >= m_impl->m_depth)
    return 0.0f;

  return m_impl->m_heightMap[x][z];
}

std::shared_ptr<VertexArrayObject> Terrain::GetVAO() const {
  return m_impl->m_vao;
}
float Terrain::GetScale() const { return m_impl->m_scale; }
float Terrain::GetMaxHeight() const { return m_impl->m_maxHeight; }
uint32_t Terrain::GetSeed() const { return m_impl->m_seed; }
float Terrain::GetFrequency() const { return m_impl->m_frequency; }
int Terrain::GetOctaves() const { return m_impl->m_octaves; }
float Terrain::GetPersistence() const { return m_impl->m_persistence; }
float Terrain::GetLacunarity() const { return m_impl->m_lacunarity; }
glm::vec3 Terrain::GetPosition() const { return m_impl->m_position; }
void Terrain::SetPosition(const glm::vec3 &pos) { m_impl->m_position = pos; }

} // namespace engine