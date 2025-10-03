#include "VertexArrayObject.h"

namespace engine {
std::shared_ptr<VertexArrayObject> CreateOpenGLVertexArrayObject();
}

namespace engine {
std::shared_ptr<VertexArrayObject> VertexArrayObject::Create() {
  return CreateOpenGLVertexArrayObject();
}
} // namespace engine
