#include "OpenGLIndexBuffer.h"

namespace engine {
std::unique_ptr<IndexBuffer> CreateOpenGLIndexBuffer(uint32_t *indices,
                                                     uint32_t count) {
  return std::make_unique<OpenGLIndexBuffer>(indices, count);
}

OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t *indices, uint32_t count) {
  m_count = count;
  glGenBuffers(1, &m_id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices,
               GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer() { glDeleteBuffers(1, &m_id); }

void OpenGLIndexBuffer::Bind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
}

void OpenGLIndexBuffer::Unbind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

} // namespace engine
