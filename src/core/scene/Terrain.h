#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <string>
namespace engine {
class VertexArrayObject;
struct TerrainGenerationSettings {
  int width = 256;
  int depth = 256;
  uint32_t seed = 12345;
  float maxHeight = 50.0f;

  float frequency = 0.02f;
  int octaves = 6;
  float persistence = 0.5f;
  float lacunarity = 2.0f;
  bool smooth = true;
  float scale = 1.0f;
};
class Terrain {
public:
  Terrain();
  ~Terrain();

  void GenerateRandom(const TerrainGenerationSettings &settings);
  bool LoadHeightmap(const std::string &path);

  float GetHeightAt(float worldX, float worldZ) const;
  glm::vec3 GetNormalAt(float worldX, float worldZ) const;

  std::shared_ptr<VertexArrayObject> GetVAO() const;
  int GetWidth() const;
  int GetDepth() const;
  float GetScale() const;

private:
  class PImpl;
  std::unique_ptr<PImpl> m_impl;
};
} // namespace engine