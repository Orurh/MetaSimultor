// #include "core/scene/Terrain.h"
// #include "mocks/MockGraphics.h"
// #include <gtest/gtest.h>

// // === Переопределяем Create() только для тестов ===
// namespace engine {

// std::shared_ptr<VertexBuffer> VertexBuffer::Create(float *vertices,
//                                                    int32_t size) {
//   return std::make_shared<MockVertexBuffer>(vertices, size);
// }

// std::shared_ptr<IndexBuffer> IndexBuffer::Create(uint32_t *data,
//                                                  uint32_t count) {
//   return std::make_shared<MockIndexBuffer>(data, count);
// }

// std::shared_ptr<VertexArrayObject> VertexArrayObject::Create() {
//   return std::make_shared<MockVertexArrayObject>();
// }

// std::shared_ptr<Texture> Texture::Create(const std::string &path) {
//   return std::make_shared<MockTexture>(path);
// }

// } // namespace engine
// using namespace engine;

// TEST(TerrainTest, GenerateRandom_BasicProperties) {
//   Terrain terrain;

//   TerrainGenerationSettings settings;
//   settings.width = 64;
//   settings.depth = 64;
//   settings.seed = 42;
//   settings.maxHeight = 100.0f;
//   settings.frequency = 0.05f;
//   settings.octaves = 4;
//   settings.persistence = 0.5f;
//   settings.lacunarity = 2.0f;

//   terrain.GenerateRandom(settings);
//   ASSERT_NE(terrain.GetVAO(), nullptr);
//   EXPECT_EQ(terrain.GetWidth(), 64);
//   EXPECT_EQ(terrain.GetDepth(), 64);

//   for (int z = 0; z < settings.depth; ++z) {
//     for (int x = 0; x < settings.width; ++x) {
//       float worldX = x * 1.0f; // scale = 1.0
//       float worldZ = z * 1.0f;
//       float height = terrain.GetHeightAt(worldX, worldZ);
//       EXPECT_GE(height, 0.0f) << "Height below 0 at (" << x << ", " << z << ")";
//       EXPECT_LE(height, settings.maxHeight)
//           << "Height above max at (" << x << ", " << z << ")";
//     }
//   }
// }

// TEST(TerrainTest, GenerateRandom_DeterministicWithSeed) {
//   Terrain terrain1, terrain2;

//   TerrainGenerationSettings settings;
//   settings.width = 32;
//   settings.depth = 32;
//   settings.seed = 999;
//   settings.maxHeight = 50.0f;

//   terrain1.GenerateRandom(settings);
//   terrain2.GenerateRandom(settings);

//   for (int i = 0; i < 10; ++i) {
//     float x = i * 3.0f;
//     float z = i * 2.0f;
//     EXPECT_FLOAT_EQ(terrain1.GetHeightAt(x, z), terrain2.GetHeightAt(x, z))
//         << "Seed не детерминирован в точке (" << x << ", " << z << ")";
//   }
// }

// TEST(TerrainTest, GenerateRandom_DifferentSeeds_DifferentResult) {
//   Terrain terrain1, terrain2;

//   TerrainGenerationSettings s1, s2;
//   s1.width = s2.width = 16;
//   s1.depth = s2.depth = 16;
//   s1.maxHeight = s2.maxHeight = 100.0f;
//   s1.seed = 111;
//   s2.seed = 222;

//   terrain1.GenerateRandom(s1);
//   terrain2.GenerateRandom(s2);

//   float h1 = terrain1.GetHeightAt(8, 8);
//   float h2 = terrain2.GetHeightAt(8, 8);

//   EXPECT_NE(h1, h2) << "Разные seed дали одинаковый террейн";
// }

// TEST(TerrainMeshTest, VertexCount_MatchesGrid) {
//   Terrain terrain;
//   TerrainGenerationSettings s{{0, 0, 0}, 7, 4};
//   terrain.GenerateRandom(s);

//   auto vao = terrain.GetVAO();
//   const auto &vbs = vao->GetVertexBuffers();
//   ASSERT_EQ(vbs.size(), 1);

//   uint32_t vertexCount = vbs[0]->GetSize() / (8 * sizeof(float));
//   EXPECT_EQ(vertexCount, 7 * 4);
// }