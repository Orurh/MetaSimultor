#include "VertexBuffer.h"

namespace engine {
std::unique_ptr<VertexBuffer> CreateOpenGLVertexBuffer(int32_t size);
std::unique_ptr<VertexBuffer> CreateOpenGLVertexBuffer(float *vertices,
                                                       int32_t size);
} // namespace engine

namespace engine {
std::shared_ptr<VertexBuffer> VertexBuffer::Create(int32_t size) {
  return CreateOpenGLVertexBuffer(size);
}
std::shared_ptr<VertexBuffer> VertexBuffer::Create(float *vertices,
                                                   int32_t size) {
  return CreateOpenGLVertexBuffer(vertices, size);
}
} // namespace engine