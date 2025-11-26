#pragma once
#include <glm/glm.hpp>
struct WindConfig {
  glm::vec3 direction = glm::vec3(1.0f, 0.0f, 0.0f);  // Направление ветра
  float strength = 3.0f;                             // Сила ветра (0-1)
  float gustiness = 0.3f;                             // Порывистость
  float change_speed = 0.1f;                          // Скорость изменения

  glm::vec3 getCurrentWind(float time) const {
    glm::vec3 base_wind = glm::normalize(direction) * strength;

    float noise_x = sin(time * change_speed * 2.0f) * gustiness;
    float noise_z = cos(time * change_speed * 1.7f) * gustiness;

    return base_wind + glm::vec3(noise_x, 0.0f, noise_z);
  }
  void setWind(const glm::vec3 &wind) {
    strength = glm::length(wind);
    direction = strength > 0.0f ? glm::normalize(wind) : glm::vec3(1.0f, 0.0f, 0.0f);
  }
  glm::vec3 getBaseWind() const { return glm::normalize(direction) * strength; }
};