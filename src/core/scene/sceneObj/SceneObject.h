#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace engine {

class Model; // forward-declare, чтобы не тянуть graphics/Model.h

struct ObjectModels {
  std::shared_ptr<Model> m_normal;  // обычное состояние
  std::shared_ptr<Model> m_burning; // горит
  std::shared_ptr<Model> m_burned;  // сгоревшее
  void Cleanup() {
    m_normal.reset();
    m_burning.reset();
    m_burned.reset();
  }
};

struct SceneObject {
  int m_agent_id = -1;       // ID агента в симуляции
  std::string m_object_type; // ключ префаба/модели
  glm::vec3 m_position{0.F}; // мировая позиция
  glm::vec3 m_scale{1.F};    // масштаб модели
  bool m_is_visible = true;
};

} // namespace engine
