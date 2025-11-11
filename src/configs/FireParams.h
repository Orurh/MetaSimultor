#pragma once
#include <glm/glm.hpp>
#include <string>
struct FireParams {
  float baseIntensity = 1.0f;
  float particleSize = 2.0f;
  float sizeVariation = 0.3f;

  // Время и анимаци
  float animationSpeed = 1.5f;
  float particleLifetime = 3.0f;
  float spawnRate = 200.0f; // базовое количество частиц

  // Движение
  float riseSpeed = 2.0f;
  float turbulence = 0.8f;
  float windInfluence = 2.0f;

  // Визуальные эффекты
  float redIntensity = 0.8f;
  float yellowReduction = 0.3f;
  float alphaMultiplier = 1.5f;

  // Распределение частиц
  float horizontalSpread = 0.8f;
  float verticalSpread = 0.5f;

  // Цветовые корректировки
  glm::vec3 baseColor = glm::vec3(1.0f, 0.8f, 0.3f);
  glm::vec3 endColor = glm::vec3(1.0f, 0.3f, 0.1f);

  float textureInfluence = 0.7f;
  std::string fireTexturePath = "res/textures/fire_texture3.png";
};