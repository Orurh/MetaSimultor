#include "OpenGLVertexBuffer.h"

namespace engine {
std::unique_ptr<VertexBuffer> CreateOpenGLVertexBuffer(int32_t size) {
  return std::make_unique<OpenGLVertexBuffer>(size);
}
std::unique_ptr<VertexBuffer> CreateOpenGLVertexBuffer(float *vertices,
                                                       int32_t size) {
  return std::make_unique<OpenGLVertexBuffer>(vertices, size);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(int32_t size) {
  glGenBuffers(1, &m_id);
  glBindBuffer(GL_ARRAY_BUFFER, m_id);
  glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}
OpenGLVertexBuffer::OpenGLVertexBuffer(float *vertices, int32_t size) {
  glGenBuffers(1, &m_id);
  glBindBuffer(GL_ARRAY_BUFFER, m_id);
  glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}
OpenGLVertexBuffer::~OpenGLVertexBuffer() { glDeleteBuffers(1, &m_id); }

void OpenGLVertexBuffer::Bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_id); }

void OpenGLVertexBuffer::Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void OpenGLVertexBuffer::SetData(const void *data, uint32_t size) {
  glBindBuffer(GL_ARRAY_BUFFER, m_id);
  glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void OpenGLVertexBuffer::SetLayout(const BufferLayout &layout) {
  m_layout = layout;
}

const BufferLayout &OpenGLVertexBuffer::GetLayout() const { return m_layout; }
} // namespace engine