#include "Terrain.h"
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include <glm/gtc/noise.hpp>
#include <random>
#include "graphics/IndexBuffer.h"
#include "graphics/Texture.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"
namespace engine {
class Terrain::PImpl {
 public:
  int m_width = 0;  // на всякий случай
  int m_depth = 0;  // тоже
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

  TerrainTextureSettings m_textureSettings;
  std::shared_ptr<Texture> m_diffuseTexture;

  [[nodiscard]] static float FBM(glm::vec2 p, const TerrainGenerationSettings& s) {
    float total = 0.0F;
    float amplitude = 1.0F;
    float freq = s.frequency;
    float maxValue = 0.0F;

    for (int i = 0; i < s.octaves; ++i) {
      glm::vec2 coord = p * freq;
      total += glm::perlin(coord) * amplitude;
      maxValue += amplitude;
      amplitude *= s.persistence;
      freq *= s.lacunarity;
    }
    return total / maxValue;
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
    m_textureSettings = s.textureSettings;

    m_heightMap.assign(m_width, std::vector<float>(m_depth, 0.0F));

    // добавил генерацию тут, не в цикле (на карту а не точку)
    std::mt19937 gen(m_seed);
    std::uniform_real_distribution<float> dist(0.0F, 10000.0F);
    glm::vec2 offset(dist(gen), dist(gen));

    for (int z = 0; z < m_depth; ++z) {
      for (int x = 0; x < m_width; ++x) {
        glm::vec2 pos = glm::vec2(x, z) * m_scale + offset;
        float h = (FBM(pos, s) + 1.0F) * 0.5F;
        m_heightMap[x][z] = h * m_maxHeight;
      }
    }
  }

  void GenerateMesh() {
    // защита, а то без генерации терреин бывало падало при добавлении пребафов (может и не тут причина)
    if (m_width <= 0 || m_depth <= 0) {
      m_vao.reset();
      return;
    }
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    // гггг
    vertices.reserve(static_cast<size_t>(m_width) * m_depth * 8);
    indices.reserve(static_cast<size_t>(m_width - 1) * (m_depth - 1) * 6);

    float textureScale = m_textureSettings.textureScale;

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
          normal = glm::normalize(glm::vec3(hL - hR, 2.0F * m_scale, hD - hU));
        }

        float u = (static_cast<float>(x) / (m_width - 1)) * textureScale;
        float v = (static_cast<float>(z) / (m_depth - 1)) * textureScale;

        vertices.insert(vertices.end(), {worldX, y, worldZ, normal.x, normal.y, normal.z, u, v});
      }
    }

    for (int z = 0; z < m_depth - 1; ++z) {
      for (int x = 0; x < m_width - 1; ++x) {
        uint32_t i = x + (z * m_width);
        indices.insert(indices.end(), {i, i + m_width, i + 1, i + 1, i + m_width, i + m_width + 1});
      }
    }

    auto vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
    vbo->SetLayout({{ShaderDataType::Float3, "a_Position"},
                    {ShaderDataType::Float3, "a_Normal"},
                    {ShaderDataType::Float2, "a_TexCoord"}});

    auto ibo = IndexBuffer::Create(indices.data(), indices.size());
    m_vao = VertexArrayObject::Create();
    m_vao->AddVertexBuffer(vbo);
    m_vao->SetIndexBuffer(ibo);
  }

  bool LoadTexture(const std::string &path) {
    m_diffuseTexture = Texture::Create(path);
    if (m_diffuseTexture && m_diffuseTexture->GetRendererID() != 0) {
      m_textureSettings.diffuseMap = path;
      return true;
    }
    m_diffuseTexture.reset();
    return false;
  }

  void UnloadTexture() {
    m_diffuseTexture.reset();
    m_textureSettings.diffuseMap.clear();
  }
  void Cleanup() {
    m_vao.reset();
    m_diffuseTexture.reset();
  }
};

Terrain::Terrain() : m_impl(std::make_unique<PImpl>()) {}
Terrain::~Terrain() = default;

void Terrain::GenerateRandom(const TerrainGenerationSettings& s) {
  m_impl->GenerateHeightmapRandom(s);
  m_impl->GenerateMesh();

  if (!s.textureSettings.diffuseMap.empty()) {
    m_impl->LoadTexture(s.textureSettings.diffuseMap);
  }
}

int Terrain::GetWidth() const {
  return m_impl->m_width;
}
int Terrain::GetDepth() const {
  return m_impl->m_depth;
}

float Terrain::GetHeightAt(float worldX, float worldZ) const {
  // билинейная интерполяция, а то скачет
  float localX = (worldX - m_impl->m_position.x) / m_impl->m_scale;
  float localZ = (worldZ - m_impl->m_position.z) / m_impl->m_scale;
  int x = static_cast<int>(localX);
  int z = static_cast<int>(localZ);
  
  // возвращаю y если за терраин вышли
  if (x < 0 || x >= m_impl->m_width - 1 || z < 0 || z >= m_impl->m_depth - 1) {
    return m_impl->m_position.y;
}
  float fracX = localX - x;
  float fracZ = localZ - z;
  float h00 = m_impl->m_heightMap[x][z];
  float h10 = m_impl->m_heightMap[x + 1][z];
  float h01 = m_impl->m_heightMap[x][z + 1];
  float h11 = m_impl->m_heightMap[x + 1][z + 1];
  float h0 = (h00 * (1 - fracX)) + (h10 * fracX);
  float h1 = (h01 * (1 - fracX)) + (h11 * fracX);
  return m_impl->m_position.y + (h0 * (1 - fracZ) + h1 * fracZ);
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
void Terrain::Cleanup() { m_impl->Cleanup(); }
std::shared_ptr<Texture> Terrain::GetDiffuseTexture() const {
  return m_impl->m_diffuseTexture;
}

const TerrainTextureSettings &Terrain::GetTextureSettings() const {
  return m_impl->m_textureSettings;
}
bool Terrain::LoadTexture(const std::string &path) {
  return m_impl->LoadTexture(path);
}

void Terrain::UnloadTexture() { m_impl->UnloadTexture(); }

}  // namespace engine