#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace engine {

struct ParticleEffect {
  // === MAIN MODULE ===
  struct MainModule {
    float duration = 5.0f;
    bool looping = true;
    bool prewarm = false;
    float startDelay = 0.0f;

    // Start values
    float startLifetime = 5.0f;
    glm::vec3 startSpeed = {0.0f, 5.0f, 0.0f};
    glm::vec3 startSize = {1.0f, 1.0f, 1.0f};
    bool use3DSize = false;
    glm::vec3 startRotation = {0.0f, 0.0f, 0.0f};
    bool use3DRotation = false;
    float flipRotation = 0.0f;
    glm::vec4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    float gravityModifier = 0.0f;

    // Simulation
    int simulationSpace = 0; // 0=Local, 1=World, 2=Custom
    float simulationSpeed = 1.0f;
    int deltaTimeMode = 0; // 0=Scaled, 1=Unscaled
    int scalingMode = 1;   // 0=Hierarchy, 1=Local, 2=Shape
    bool playOnAwake = true;
    int emitterVelocityMode = 0; // 0=Transform, 1=Rigidbody
    uint32_t maxParticles = 1000;
    bool autoRandomSeed = true;
    uint32_t randomSeed = 0;
    int stopAction = 0; // 0=Disable, 1=Destroy, 2=Callback
    int cullingMode = 0;
    int ringBufferMode = 0;
  } main;

  // === EMISSION MODULE ===
  struct EmissionModule {
    bool enabled = true;
    float rateOverTime = 10.0f;
    float rateOverDistance = 0.0f;

    struct Burst {
      float time = 0.0f;
      uint32_t count = 10;
      uint32_t cycles = 1;
      float interval = 1.0f;
      float probability = 1.0f;
    };
    std::vector<Burst> bursts;
  } emission;

  // === SHAPE MODULE ===
  struct ShapeModule {
    bool enabled = true;
    int shapeType = 0; // 0=Sphere, 1=Cone, 2=Box, 3=Mesh
    float radius = 1.0f;
    float angle = 25.0f;
    glm::vec3 boxSize = {1.0f, 1.0f, 1.0f};
    float arc = 360.0f;
    float randomDirection = 0.0f;
    bool alignToDirection = false;
  } shape;

  // === VELOCITY OVER LIFETIME ===
  struct VelocityOverLifetime {
    bool enabled = false;
    glm::vec3 linear = {0.0f, 0.0f, 0.0f};
    glm::vec3 orbital = {0.0f, 0.0f, 0.0f};
    glm::vec3 offset = {0.0f, 0.0f, 0.0f};
    float speedModifier = 1.0f;
  } velocityOverLifetime;

  // === LIMIT VELOCITY OVER LIFETIME ===
  struct LimitVelocityOverLifetime {
    bool enabled = false;
    float limit = 1.0f;
    float dampen = 0.5f;
    bool separateAxes = false;
    glm::vec3 limitXYZ = {1.0f, 1.0f, 1.0f};
  } limitVelocityOverLifetime;

  // === FORCE OVER LIFETIME ===
  struct ForceOverLifetime {
    bool enabled = false;
    glm::vec3 force = {0.0f, 0.0f, 0.0f};
    bool randomize = false;
    glm::vec3 randomForce = {1.0f, 1.0f, 1.0f};
  } forceOverLifetime;

  // === COLOR OVER LIFETIME ===
  struct ColorOverLifetime {
    bool enabled = false;

    struct GradientColor {
      float time;
      glm::vec4 color;
    };
    std::vector<GradientColor> gradient;
  } colorOverLifetime;

  // === SIZE OVER LIFETIME ===
  struct SizeOverLifetime {
    bool enabled = false;

    struct SizeCurve {
      float time;
      glm::vec3 size;
    };
    std::vector<SizeCurve> curve;
  } sizeOverLifetime;

  // === ROTATION OVER LIFETIME ===
  struct RotationOverLifetime {
    bool enabled = false;
    glm::vec3 angularVelocity = {0.0f, 0.0f, 0.0f};
    bool separateAxes = false;
  } rotationOverLifetime;

  // === TEXTURE SHEET ANIMATION ===
  struct TextureSheetAnimation {
    bool enabled = false;
    uint32_t numTilesX = 1;
    uint32_t numTilesY = 1;
    int animationType = 0; // 0=WholeSheet, 1=SingleRow
    uint32_t rowIndex = 0;
    float frameOverTime = 1.0f;
    uint32_t startFrame = 0;
    uint32_t cycles = 1;
  } textureSheetAnimation;

  // === RENDERER MODULE ===
  struct RendererModule {
    std::string texturePath;
    int renderMode = 0; // 0=Billboard, 1=StretchedBillboard,
                        // 2=HorizontalBillboard, 3=VerticalBillboard, 4=Mesh
    glm::vec3 normalDirection = {0.0f, 1.0f, 0.0f};
    float lengthScale = 1.0f;
    float speedScale = 1.0f;
    glm::vec4 materialColor = {1.0f, 1.0f, 1.0f, 1.0f};
    bool castShadows = false;
    bool receiveShadows = false;
    int sortMode =
        0; // 0=None, 1=ByDistance, 2=OldestInFront, 3=YoungestInFront
  } renderer;

  // Методы
  void SaveToFile(const std::string &path);
  void LoadFromFile(const std::string &path);

  static std::shared_ptr<ParticleEffect> CreateDefault();
  static std::shared_ptr<ParticleEffect> CreateFire();
  static std::shared_ptr<ParticleEffect> CreateSmoke();
  static std::shared_ptr<ParticleEffect> CreateMagic();

  YAML::Node ToYAML() const;
  void FromYAML(const YAML::Node &node);
};

} // namespace engine