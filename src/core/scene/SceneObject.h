// scene/SceneObject.h
#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace engine {

class Model; // forward-declare, чтобы не тянуть graphics/Model.h

struct ObjectModels {
  std::shared_ptr<Model> normal;   // обычное состояние
  std::shared_ptr<Model> burning;  // горит
  std::shared_ptr<Model> burned;   // сгоревшее
};

struct SceneObject {
  int         agent_id = -1;        // ID агента в симуляции
  std::string object_type;          // ключ префаба/модели
  glm::vec3   position{0.f};        // мировая позиция
  glm::vec3   scale{1.f};           // масштаб модели
  bool        is_visible = true;
};

} // namespace engine
