#include "ParticleEffect.h"

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
  effect->main.startColor = {1.0f, 1.0f, 1.0f, 1.0f};
  effect->main.gravityModifier = 1.0f;
  effect->main.maxParticles = 1000;

  effect->emission.rateOverTime = 10.0f;

  effect->shape.shapeType = 0; // Sphere
  effect->shape.radius = 1.0f;

  return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffect::CreateFire() {
  auto effect = CreateDefault();

  effect->main.startLifetime = 2.0f;
  effect->main.startSpeed = {0.0f, 3.0f, 0.0f};
  effect->main.startColor = {1.0f, 0.3f, 0.1f, 1.0f};
  effect->main.gravityModifier = -0.5f; // Огонь поднимается

  effect->emission.rateOverTime = 50.0f;

  // Color over Lifetime - от оранжевого к прозрачному
  effect->colorOverLifetime.enabled = true;
  effect->colorOverLifetime.gradient = {{0.0f, {1.0f, 0.5f, 0.1f, 1.0f}},
                                        {0.5f, {1.0f, 0.8f, 0.2f, 0.8f}},
                                        {1.0f, {1.0f, 0.9f, 0.3f, 0.0f}}};

  // Size over Lifetime - уменьшается
  effect->sizeOverLifetime.enabled = true;
  effect->sizeOverLifetime.curve = {{0.0f, {1.0f, 1.0f, 1.0f}},
                                    {1.0f, {0.1f, 0.1f, 0.1f}}};

  //   effect->renderer.blendMode = 1; // Additive blending
  //   effect->renderer.glowIntensity = 2.0f;

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

// Реализация методов сохранения/загрузки (заглушки)
void ParticleEffect::SaveToFile(const std::string &path) {
  // TODO: реализовать сериализацию в JSON
}

void ParticleEffect::LoadFromFile(const std::string &path) {
  // TODO: реализовать десериализацию из JSON
}

} // namespace engine