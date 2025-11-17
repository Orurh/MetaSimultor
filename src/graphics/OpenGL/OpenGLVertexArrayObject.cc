#include "OpenGLVertexArrayObject.h"
#include <iostream>
namespace engine {

std::shared_ptr<VertexArrayObject> CreateOpenGLVertexArrayObject() {
  return std::make_shared<OpenGLVertexArrayObject>();
}
static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
  switch (type) {
  case ShaderDataType::Float:
    return GL_FLOAT;
  case ShaderDataType::Float2:
    return GL_FLOAT;
  case ShaderDataType::Float3:
    return GL_FLOAT;
  case ShaderDataType::Float4:
    return GL_FLOAT;
  case ShaderDataType::Mat3:
    return GL_FLOAT;
  case ShaderDataType::Mat4:
    return GL_FLOAT;
  case ShaderDataType::Int:
    return GL_INT;
  case ShaderDataType::Int2:
    return GL_INT;
  case ShaderDataType::Int3:
    return GL_INT;
  case ShaderDataType::Int4:
    return GL_INT;
  case ShaderDataType::Bool:
    return GL_BOOL;
  }
  return 0;
}
OpenGLVertexArrayObject::OpenGLVertexArrayObject() {
  glGenVertexArrays(1, &m_id);
  glBindVertexArray(m_id);
}

OpenGLVertexArrayObject::~OpenGLVertexArrayObject() {
  glDeleteVertexArrays(1, &m_id);
}

void OpenGLVertexArrayObject::Bind() const { glBindVertexArray(m_id); }

void OpenGLVertexArrayObject::Unbind() const { glBindVertexArray(0); }

void OpenGLVertexArrayObject::AddVertexBuffer(
    const std::shared_ptr<VertexBuffer> &vertexBuffer) {
  glBindVertexArray(m_id);
  vertexBuffer->Bind();
  const auto &layout = vertexBuffer->GetLayout();

  for (const auto &elem : layout) {
    switch (elem.type) {
    case ShaderDataType::Float:
    case ShaderDataType::Float2:
    case ShaderDataType::Float3:
    case ShaderDataType::Float4: {
      glEnableVertexAttribArray(m_VertexBufferIndex);
      glVertexAttribPointer(m_VertexBufferIndex, elem.GetComponentCount(),
                            ShaderDataTypeToOpenGLBaseType(elem.type),
                            elem.normalized ? GL_TRUE : GL_FALSE,
                            layout.GetStride(),
                            reinterpret_cast<void *>(elem.offset));
      m_VertexBufferIndex++;
      break;
    }

    case ShaderDataType::Int:
    case ShaderDataType::Int2:
    case ShaderDataType::Int3:
    case ShaderDataType::Int4:
    case ShaderDataType::Bool: {
      glEnableVertexAttribArray(m_VertexBufferIndex);

      glVertexAttribIPointer(m_VertexBufferIndex, elem.GetComponentCount(),
                             ShaderDataTypeToOpenGLBaseType(elem.type),
                             layout.GetStride(),
                             reinterpret_cast<void *>(elem.offset));
      m_VertexBufferIndex++;
      break;
    }

    case ShaderDataType::Mat3:
    case ShaderDataType::Mat4: {
      uint8_t count = elem.GetComponentCount();
      for (uint8_t i = 0; i < count; ++i) {
        glEnableVertexAttribArray(m_VertexBufferIndex);
        glVertexAttribPointer(
            m_VertexBufferIndex, count,
            ShaderDataTypeToOpenGLBaseType(elem.type),
            elem.normalized ? GL_TRUE : GL_FALSE, layout.GetStride(),
            reinterpret_cast<void *>(elem.offset + sizeof(float) * count * i));
        glVertexAttribDivisor(m_VertexBufferIndex, 1);
        m_VertexBufferIndex++;
      }
      break;
    }
    default:
      std::cout << "Unknown ShaderDataType!" << std::endl;
    }
  }
  m_vertexBuffers.push_back(vertexBuffer);
}

void OpenGLVertexArrayObject::SetIndexBuffer(
    const std::shared_ptr<IndexBuffer> &indexBuffer) {
  glBindVertexArray(m_id);
  indexBuffer->Bind();
  m_indexBuffer = indexBuffer;
}
} // namespace engine