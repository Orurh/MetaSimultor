#include "graphics/VertexBuffer.h"
#include <glad/glad.h>

namespace engine {
class OpenGLVertexBuffer final : public VertexBuffer {
public:
  OpenGLVertexBuffer(int32_t size);
  OpenGLVertexBuffer(float *vertices, int32_t size);
  virtual ~OpenGLVertexBuffer();
  virtual void SetData(const void *data, uint32_t size) override;
  virtual void SetLayout(const BufferLayout &layout) override;
  virtual const BufferLayout &GetLayout() const override;
  void Bind() const override;
  void Unbind() const override;
  size_t GetSize() const override {
        return m_size;
  }
private:
  BufferLayout m_layout;
  GLuint m_id;
  size_t m_size; 

};
} // namespace engine
