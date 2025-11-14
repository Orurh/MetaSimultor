#include "ParticleEffect.h"
#include <fstream>
namespace engine {

// Реализация фабричных методов
std::shared_ptr<ParticleEffect> ParticleEffect::CreateDefault() {
  auto effect = std::make_shared<ParticleEffect>();

  // Настройки по умолчанию
  effect->main.duration = 5.0f;
  effect->main.looping = true;
  effect->main.startLifetime = 3.0f;
  effect->main.startSpeed = {0.0f, 5.0f, 0.0f};
  effect->main.startSize = {1.0f, 1.0f, 1.0f};
  effect->main.startColor = {1.0f, 0.0f, 0.0f, 1.0f};
  effect->main.gravityModifier = 1.0f;
  effect->main.maxParticles = 1000;

  effect->emission.rateOverTime = 10.0f;

  effect->shape.shapeType = 0;
  effect->shape.radius = 1.0f;

  return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffect::CreateFire() {
  static auto effect = []() {
    auto e = std::make_shared<ParticleEffect>();
    e->LoadFromFile("res/effects/Fire.yaml");
    return e;
  }();
  return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffect::CreateSmoke() {
  auto effect = CreateDefault();

  effect->main.startLifetime = 4.0f;
  effect->main.startSpeed = {0.0f, 2.0f, 0.0f};
  effect->main.startColor = {0.3f, 0.3f, 0.3f, 0.8f};
  effect->main.startSize = {2.0f, 2.0f, 2.0f};
  effect->main.gravityModifier = 0.2f;

  effect->emission.rateOverTime = 5.0f;

  // Size over Lifetime - увеличивается
  effect->sizeOverLifetime.enabled = true;
  effect->sizeOverLifetime.curve = {{0.0f, {1.0f, 1.0f, 1.0f}},
                                    {1.0f, {3.0f, 3.0f, 3.0f}}};

  // Color over Lifetime - становится прозрачнее
  effect->colorOverLifetime.enabled = true;
  effect->colorOverLifetime.gradient = {{0.0f, {0.2f, 0.2f, 0.2f, 0.8f}},
                                        {1.0f, {0.1f, 0.1f, 0.1f, 0.0f}}};

  // effect->renderer.softness = 0.9f;

  return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffect::CreateMagic() {
  auto effect = CreateDefault();

  effect->main.startLifetime = 2.0f;
  effect->main.startSpeed = {0.0f, 1.0f, 0.0f};
  effect->main.startColor = {0.5f, 0.2f, 1.0f, 1.0f};
  effect->main.gravityModifier = 0.0f;

  effect->emission.rateOverTime = 20.0f;

  // Velocity over Lifetime - вращение
  effect->velocityOverLifetime.enabled = true;
  effect->velocityOverLifetime.orbital = {2.0f, 0.0f, 0.0f};

  // Color over Lifetime - меняет цвет
  effect->colorOverLifetime.enabled = true;
  effect->colorOverLifetime.gradient = {{0.0f, {0.5f, 0.2f, 1.0f, 1.0f}},
                                        {0.5f, {0.2f, 0.8f, 1.0f, 0.5f}},
                                        {1.0f, {1.0f, 1.0f, 0.2f, 0.0f}}};

  //   effect->renderer.blendMode = 1; // Additive
  //   effect->renderer.glowIntensity = 1.5f;

  return effect;
}

YAML::Node ParticleEffect::ToYAML() const {
  YAML::Node node;

  // Main Module
  node["main"]["duration"] = main.duration;
  node["main"]["looping"] = main.looping;
  node["main"]["startDelay"] = main.startDelay;
  node["main"]["startLifetime"] = main.startLifetime;
  node["main"]["startSpeed"] = std::vector<float>{
      main.startSpeed.x, main.startSpeed.y, main.startSpeed.z};
  node["main"]["startSize"] =
      std::vector<float>{main.startSize.x, main.startSize.y, main.startSize.z};
  node["main"]["use3DSize"] = main.use3DSize;
  node["main"]["startRotation"] = std::vector<float>{
      main.startRotation.x, main.startRotation.y, main.startRotation.z};
  node["main"]["use3DRotation"] = main.use3DRotation;
  node["main"]["startColor"] =
      std::vector<float>{main.startColor.r, main.startColor.g,
                         main.startColor.b, main.startColor.a};
  node["main"]["gravityModifier"] = main.gravityModifier;
  node["main"]["simulationSpace"] = main.simulationSpace;
  node["main"]["simulationSpeed"] = main.simulationSpeed;
  node["main"]["maxParticles"] = main.maxParticles;

  // Emission Module
  node["emission"]["enabled"] = emission.enabled;
  node["emission"]["rateOverTime"] = emission.rateOverTime;
  node["emission"]["rateOverDistance"] = emission.rateOverDistance;

  for (const auto &burst : emission.bursts) {
    YAML::Node burstNode;
    burstNode["time"] = burst.time;
    burstNode["count"] = burst.count;
    burstNode["cycles"] = burst.cycles;
    burstNode["interval"] = burst.interval;
    burstNode["probability"] = burst.probability;
    node["emission"]["bursts"].push_back(burstNode);
  }

  // Shape Module
  node["shape"]["enabled"] = shape.enabled;
  node["shape"]["shapeType"] = shape.shapeType;
  node["shape"]["radius"] = shape.radius;
  node["shape"]["angle"] = shape.angle;
  node["shape"]["boxSize"] =
      std::vector<float>{shape.boxSize.x, shape.boxSize.y, shape.boxSize.z};
  node["shape"]["arc"] = shape.arc;
  node["shape"]["randomDirection"] = shape.randomDirection;
  node["shape"]["alignToDirection"] = shape.alignToDirection;

  // Velocity Over Lifetime
  node["velocityOverLifetime"]["enabled"] = velocityOverLifetime.enabled;
  node["velocityOverLifetime"]["linear"] = std::vector<float>{
      velocityOverLifetime.linear.x, velocityOverLifetime.linear.y,
      velocityOverLifetime.linear.z};
  node["velocityOverLifetime"]["orbital"] = std::vector<float>{
      velocityOverLifetime.orbital.x, velocityOverLifetime.orbital.y,
      velocityOverLifetime.orbital.z};
  node["velocityOverLifetime"]["speedModifier"] =
      velocityOverLifetime.speedModifier;

  // Force Over Lifetime
  node["forceOverLifetime"]["enabled"] = forceOverLifetime.enabled;
  node["forceOverLifetime"]["force"] =
      std::vector<float>{forceOverLifetime.force.x, forceOverLifetime.force.y,
                         forceOverLifetime.force.z};
  node["forceOverLifetime"]["randomize"] = forceOverLifetime.randomize;
  node["forceOverLifetime"]["randomForce"] = std::vector<float>{
      forceOverLifetime.randomForce.x, forceOverLifetime.randomForce.y,
      forceOverLifetime.randomForce.z};

  // Limit Velocity Over Lifetime
  node["limitVelocityOverLifetime"]["enabled"] =
      limitVelocityOverLifetime.enabled;
  node["limitVelocityOverLifetime"]["limit"] = limitVelocityOverLifetime.limit;
  node["limitVelocityOverLifetime"]["dampen"] =
      limitVelocityOverLifetime.dampen;
  node["limitVelocityOverLifetime"]["separateAxes"] =
      limitVelocityOverLifetime.separateAxes;
  node["limitVelocityOverLifetime"]["limitXYZ"] =
      std::vector<float>{limitVelocityOverLifetime.limitXYZ.x,
                         limitVelocityOverLifetime.limitXYZ.y,
                         limitVelocityOverLifetime.limitXYZ.z};

  // Color Over Lifetime
  node["colorOverLifetime"]["enabled"] = colorOverLifetime.enabled;
  for (const auto &gradient : colorOverLifetime.gradient) {
    YAML::Node gradientNode;
    gradientNode["time"] = gradient.time;
    gradientNode["color"] = std::vector<float>{
        gradient.color.r, gradient.color.g, gradient.color.b, gradient.color.a};
    node["colorOverLifetime"]["gradient"].push_back(gradientNode);
  }

  // Size Over Lifetime
  node["sizeOverLifetime"]["enabled"] = sizeOverLifetime.enabled;
  for (const auto &curve : sizeOverLifetime.curve) {
    YAML::Node curveNode;
    curveNode["time"] = curve.time;
    curveNode["size"] =
        std::vector<float>{curve.size.x, curve.size.y, curve.size.z};
    node["sizeOverLifetime"]["curve"].push_back(curveNode);
  }

  // Rotation Over Lifetime
  node["rotationOverLifetime"]["enabled"] = rotationOverLifetime.enabled;
  node["rotationOverLifetime"]["angularVelocity"] =
      std::vector<float>{rotationOverLifetime.angularVelocity.x,
                         rotationOverLifetime.angularVelocity.y,
                         rotationOverLifetime.angularVelocity.z};
  node["rotationOverLifetime"]["separateAxes"] =
      rotationOverLifetime.separateAxes;

  // Renderer Module
  node["renderer"]["texturePath"] = renderer.texturePath;
  node["renderer"]["renderMode"] = renderer.renderMode;
  node["renderer"]["normalDirection"] =
      std::vector<float>{renderer.normalDirection.x, renderer.normalDirection.y,
                         renderer.normalDirection.z};
  node["renderer"]["materialColor"] =
      std::vector<float>{renderer.materialColor.r, renderer.materialColor.g,
                         renderer.materialColor.b, renderer.materialColor.a};
  node["renderer"]["castShadows"] = renderer.castShadows;
  node["renderer"]["receiveShadows"] = renderer.receiveShadows;

  return node;
}

void ParticleEffect::FromYAML(const YAML::Node &node) {
  // Main Module
  if (node["main"]) {
    main.duration = node["main"]["duration"].as<float>();
    main.looping = node["main"]["looping"].as<bool>();
    main.startDelay = node["main"]["startDelay"].as<float>();
    main.startLifetime = node["main"]["startLifetime"].as<float>();

    auto startSpeed = node["main"]["startSpeed"].as<std::vector<float>>();
    main.startSpeed = {startSpeed[0], startSpeed[1], startSpeed[2]};

    auto startSize = node["main"]["startSize"].as<std::vector<float>>();
    main.startSize = {startSize[0], startSize[1], startSize[2]};

    main.use3DSize = node["main"]["use3DSize"].as<bool>();

    auto startRotation = node["main"]["startRotation"].as<std::vector<float>>();
    main.startRotation = {startRotation[0], startRotation[1], startRotation[2]};

    main.use3DRotation = node["main"]["use3DRotation"].as<bool>();

    auto startColor = node["main"]["startColor"].as<std::vector<float>>();
    main.startColor = {startColor[0], startColor[1], startColor[2],
                       startColor[3]};

    main.gravityModifier = node["main"]["gravityModifier"].as<float>();
    main.simulationSpace = node["main"]["simulationSpace"].as<int>();
    main.simulationSpeed = node["main"]["simulationSpeed"].as<float>();
    main.maxParticles = node["main"]["maxParticles"].as<uint32_t>();
  }

  // Emission Module
  if (node["emission"]) {
    emission.enabled = node["emission"]["enabled"].as<bool>();
    emission.rateOverTime = node["emission"]["rateOverTime"].as<float>();
    emission.rateOverDistance =
        node["emission"]["rateOverDistance"].as<float>();

    emission.bursts.clear();
    if (node["emission"]["bursts"]) {
      for (const auto &burstNode : node["emission"]["bursts"]) {
        EmissionModule::Burst burst;
        burst.time = burstNode["time"].as<float>();
        burst.count = burstNode["count"].as<uint32_t>();
        burst.cycles = burstNode["cycles"].as<uint32_t>();
        burst.interval = burstNode["interval"].as<float>();
        burst.probability = burstNode["probability"].as<float>();
        emission.bursts.push_back(burst);
      }
    }
  }

  // Shape Module
  if (node["shape"]) {
    shape.enabled = node["shape"]["enabled"].as<bool>();
    shape.shapeType = node["shape"]["shapeType"].as<int>();
    shape.radius = node["shape"]["radius"].as<float>();
    shape.angle = node["shape"]["angle"].as<float>();

    auto boxSize = node["shape"]["boxSize"].as<std::vector<float>>();
    shape.boxSize = {boxSize[0], boxSize[1], boxSize[2]};

    shape.arc = node["shape"]["arc"].as<float>();
    shape.randomDirection = node["shape"]["randomDirection"].as<float>();
    shape.alignToDirection = node["shape"]["alignToDirection"].as<bool>();
  }

  // Velocity Over Lifetime
  if (node["velocityOverLifetime"]) {
    velocityOverLifetime.enabled =
        node["velocityOverLifetime"]["enabled"].as<bool>();

    auto linear =
        node["velocityOverLifetime"]["linear"].as<std::vector<float>>();
    velocityOverLifetime.linear = {linear[0], linear[1], linear[2]};

    auto orbital =
        node["velocityOverLifetime"]["orbital"].as<std::vector<float>>();
    velocityOverLifetime.orbital = {orbital[0], orbital[1], orbital[2]};

    velocityOverLifetime.speedModifier =
        node["velocityOverLifetime"]["speedModifier"].as<float>();
  }
  if (node["forceOverLifetime"]) {
    forceOverLifetime.enabled = node["forceOverLifetime"]["enabled"].as<bool>();

    auto force = node["forceOverLifetime"]["force"].as<std::vector<float>>();
    forceOverLifetime.force = {force[0], force[1], force[2]};

    forceOverLifetime.randomize =
        node["forceOverLifetime"]["randomize"].as<bool>();
    auto randomForce =
        node["forceOverLifetime"]["randomForce"].as<std::vector<float>>();
    forceOverLifetime.randomForce = {randomForce[0], randomForce[1],
                                     randomForce[2]};
  }

  // Limit Velocity Over Lifetime
  if (node["limitVelocityOverLifetime"]) {
    limitVelocityOverLifetime.enabled =
        node["limitVelocityOverLifetime"]["enabled"].as<bool>();
    limitVelocityOverLifetime.limit =
        node["limitVelocityOverLifetime"]["limit"].as<float>();
    limitVelocityOverLifetime.dampen =
        node["limitVelocityOverLifetime"]["dampen"].as<float>();
    limitVelocityOverLifetime.separateAxes =
        node["limitVelocityOverLifetime"]["separateAxes"].as<bool>();

    auto limitXYZ =
        node["limitVelocityOverLifetime"]["limitXYZ"].as<std::vector<float>>();
    limitVelocityOverLifetime.limitXYZ = {limitXYZ[0], limitXYZ[1],
                                          limitXYZ[2]};
  }
  // Color Over Lifetime
  if (node["colorOverLifetime"]) {
    colorOverLifetime.enabled = node["colorOverLifetime"]["enabled"].as<bool>();

    colorOverLifetime.gradient.clear();
    if (node["colorOverLifetime"]["gradient"]) {
      for (const auto &gradientNode : node["colorOverLifetime"]["gradient"]) {
        ColorOverLifetime::GradientColor gradient;
        gradient.time = gradientNode["time"].as<float>();
        auto color = gradientNode["color"].as<std::vector<float>>();
        gradient.color = {color[0], color[1], color[2], color[3]};
        colorOverLifetime.gradient.push_back(gradient);
      }
    }
  }

  // Size Over Lifetime
  if (node["sizeOverLifetime"]) {
    sizeOverLifetime.enabled = node["sizeOverLifetime"]["enabled"].as<bool>();

    sizeOverLifetime.curve.clear();
    if (node["sizeOverLifetime"]["curve"]) {
      for (const auto &curveNode : node["sizeOverLifetime"]["curve"]) {
        SizeOverLifetime::SizeCurve curve;
        curve.time = curveNode["time"].as<float>();
        auto size = curveNode["size"].as<std::vector<float>>();
        curve.size = {size[0], size[1], size[2]};
        sizeOverLifetime.curve.push_back(curve);
      }
    }
  }

  // Rotation Over Lifetime
  if (node["rotationOverLifetime"]) {
    rotationOverLifetime.enabled =
        node["rotationOverLifetime"]["enabled"].as<bool>();

    auto angularVelocity = node["rotationOverLifetime"]["angularVelocity"]
                               .as<std::vector<float>>();
    rotationOverLifetime.angularVelocity = {
        angularVelocity[0], angularVelocity[1], angularVelocity[2]};

    rotationOverLifetime.separateAxes =
        node["rotationOverLifetime"]["separateAxes"].as<bool>();
  }
  // Renderer Module
  if (node["renderer"]) {
    renderer.texturePath = node["renderer"]["texturePath"].as<std::string>();
    renderer.renderMode = node["renderer"]["renderMode"].as<int>();

    auto normalDirection =
        node["renderer"]["normalDirection"].as<std::vector<float>>();
    renderer.normalDirection = {normalDirection[0], normalDirection[1],
                                normalDirection[2]};

    auto materialColor =
        node["renderer"]["materialColor"].as<std::vector<float>>();
    renderer.materialColor = {materialColor[0], materialColor[1],
                              materialColor[2], materialColor[3]};

    renderer.castShadows = node["renderer"]["castShadows"].as<bool>();
    renderer.receiveShadows = node["renderer"]["receiveShadows"].as<bool>();
  }
}

void ParticleEffect::SaveToFile(const std::string &path) {
  YAML::Node node = ToYAML();
  std::ofstream fout(path);
  fout << node;
}

void ParticleEffect::LoadFromFile(const std::string &path) {
  YAML::Node node = YAML::LoadFile(path);
  FromYAML(node);
}

} // namespace engine