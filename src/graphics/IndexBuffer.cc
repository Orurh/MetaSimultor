#include "IndexBuffer.h"

namespace engine {
std::unique_ptr<IndexBuffer> CreateOpenGLIndexBuffer(uint32_t *indices,
                                                     uint32_t count);
}

namespace engine {
std::shared_ptr<IndexBuffer> IndexBuffer::Create(uint32_t *indices,
                                                 uint32_t count) {
  return CreateOpenGLIndexBuffer(indices, count);
}

} // namespace engine
