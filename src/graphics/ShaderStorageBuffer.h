#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <memory>
namespace engine {

class ShaderStorageBuffer {
  public:
  static std::shared_ptr<ShaderStorageBuffer> Create(uint32_t size, const void *data = nullptr,
                                                     uint32_t usage = GL_DYNAMIC_DRAW);
  ~ShaderStorageBuffer();
  ShaderStorageBuffer() = default;

  void Bind() const;
  void Bind(uint32_t binding) const;
  void Unbind() const;

  void SetData(const void *data, uint32_t size, uint32_t offset = 0);
  void SetSubData(const void *data, size_t offset, size_t size);
  void GetData(void *data, uint32_t size, uint32_t offset = 0) const;

  void *Map(uint32_t access = GL_READ_WRITE);
  void Unmap();

  uint32_t GetID() const { return m_rendererID; }
  uint32_t GetSize() const { return m_size; }

  private:
  uint32_t m_rendererID = 0;
  uint32_t m_size = 0;
};

}  // namespace engine