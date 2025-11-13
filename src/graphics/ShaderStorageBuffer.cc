#include "ShaderStorageBuffer.h"
#include <glad/glad.h>

namespace engine {

std::shared_ptr<ShaderStorageBuffer>
ShaderStorageBuffer::Create(uint32_t size, const void *data, uint32_t usage) {
  auto ssbo = std::make_shared<ShaderStorageBuffer>();

  glGenBuffers(1, &ssbo->m_rendererID);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo->m_rendererID);
  glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  ssbo->m_size = size;

  return ssbo;
}

ShaderStorageBuffer::~ShaderStorageBuffer() {
  if (m_rendererID) {
    glDeleteBuffers(1, &m_rendererID);
  }
}

void ShaderStorageBuffer::Bind() const {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);
}

void ShaderStorageBuffer::Bind(uint32_t binding) const {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_rendererID);
}

void ShaderStorageBuffer::Unbind() const {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::SetData(const void *data, uint32_t size,
                                  uint32_t offset) {
  Bind();
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

void ShaderStorageBuffer::GetData(void *data, uint32_t size, uint32_t offset) {
  Bind();
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

void *ShaderStorageBuffer::Map(uint32_t access) {
  Bind();
  return glMapBuffer(GL_SHADER_STORAGE_BUFFER, access);
}

void ShaderStorageBuffer::Unmap() { glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); }
} // namespace engine