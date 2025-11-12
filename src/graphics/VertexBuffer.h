#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <memory>
#include <string>
#include <vector>
namespace engine {
enum class ShaderDataType {
  Float,
  Float2,
  Float3,
  Float4,
  Int,
  Int2,
  Int3,
  Int4,
  Mat3,
  Mat4,
  Bool
};
static uint32_t ShaderDataTypeSize(ShaderDataType type) {
  switch (type) {
  case ShaderDataType::Float:
    return 4;
  case ShaderDataType::Float2:
    return 4 * 2;
  case ShaderDataType::Float3:
    return 4 * 3;
  case ShaderDataType::Float4:
    return 4 * 4;
  case ShaderDataType::Mat3:
    return 4 * 3 * 3;
  case ShaderDataType::Mat4:
    return 4 * 4 * 4;
  case ShaderDataType::Int:
    return 4;
  case ShaderDataType::Int2:
    return 4 * 2;
  case ShaderDataType::Int3:
    return 4 * 3;
  case ShaderDataType::Int4:
    return 4 * 4;
  case ShaderDataType::Bool:
    return 1;
  }
  return 0;
}
struct BufferElement {
  ShaderDataType type;
  std::string name;
  uint32_t size;
  uint32_t offset;
  bool normalized;
  BufferElement(ShaderDataType type, const std::string &name,
                bool normalized = false)
      : name(name), type(type), size(ShaderDataTypeSize(type)), offset(0),
        normalized(normalized) {}
  uint32_t GetComponentCount() const {
    switch (type) {
    case ShaderDataType::Float:
      return 1;
    case ShaderDataType::Float2:
      return 2;
    case ShaderDataType::Float3:
      return 3;
    case ShaderDataType::Float4:
      return 4;
    case ShaderDataType::Int:
      return 1;
    case ShaderDataType::Int2:
      return 2;
    case ShaderDataType::Int3:
      return 3;
    case ShaderDataType::Int4:
      return 4;
    case ShaderDataType::Mat3:
      return 3;
    case ShaderDataType::Mat4:
      return 4;
    case ShaderDataType::Bool:
      return 1;
    }
    return 0;
  }
};

class BufferLayout {
private:
  std::vector<BufferElement> m_elements;
  uint32_t m_Stride = 0;

public:
  BufferLayout() = default;
  BufferLayout(const std::initializer_list<BufferElement> &elements)
      : m_elements(elements) {
    CalculateOffsetsAndStride();
  }
  const std::vector<BufferElement> &GetElements() const { return m_elements; }
  uint32_t GetStride() const { return m_Stride; }
  // Итераторы для for-each
  std::vector<BufferElement>::iterator begin() { return m_elements.begin(); }
  std::vector<BufferElement>::iterator end() { return m_elements.end(); }
  std::vector<BufferElement>::const_iterator begin() const {
    return m_elements.begin();
  }
  std::vector<BufferElement>::const_iterator end() const {
    return m_elements.end();
  }

private:
  void CalculateOffsetsAndStride() {
    size_t offset = 0;
    m_Stride = 0;
    for (auto &element : m_elements) {
      element.offset = offset;
      offset += element.size;
      m_Stride += element.size;
    }
  }
};

class VertexBuffer {
private:
  GLuint m_id;
public:
  virtual ~VertexBuffer() = default;

  virtual void Bind() const = 0;
  virtual void Unbind() const = 0;
  virtual void SetData(const void *data, uint32_t size) = 0;
  virtual void SetLayout(const BufferLayout &layout) = 0;
  virtual size_t GetSize() const = 0;
  virtual const BufferLayout &GetLayout() const = 0;
  static std::shared_ptr<VertexBuffer> Create(int32_t size);
  static std::shared_ptr<VertexBuffer> Create(float *vertices, int32_t size);
};

} // namespace engine