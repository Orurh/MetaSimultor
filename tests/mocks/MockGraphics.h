// tests/mocks/MockGraphics.h
#pragma once
#include "graphics/IndexBuffer.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"

namespace engine {

class MockVertexBuffer : public VertexBuffer {
public:
  MockVertexBuffer(float *data, int32_t size) : m_size(size) {}
  void Bind() const override {}
  void Unbind() const override {}
  void SetData(const void *, uint32_t) override {}
  void SetLayout(const BufferLayout &layout) override { m_layout = layout; }
  const BufferLayout &GetLayout() const override { return m_layout; }
  size_t GetSize() const override { return m_size; }

private:
  size_t m_size;
  BufferLayout m_layout;
};

class MockIndexBuffer : public IndexBuffer {
public:
  MockIndexBuffer(const uint32_t *data, uint32_t count) : m_count(count) {}
  void Bind() const override {}
  void Unbind() const override {}
  uint32_t GetCount() const override { return m_count; }

private:
  uint32_t m_count;
};

class MockVertexArrayObject : public VertexArrayObject {
public:
  void Bind() const override {}
  void Unbind() const override {}
  void AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vb) override {
    m_vertexBuffers.push_back(vb);
  }
  void SetIndexBuffer(const std::shared_ptr<IndexBuffer> &ib) override {
    m_indexBuffer = ib;
  }
  const std::vector<std::shared_ptr<VertexBuffer>> &
  GetVertexBuffers() const override {
    return m_vertexBuffers;
  }
  const std::shared_ptr<IndexBuffer> &GetIndexBuffer() const override {
    return m_indexBuffer;
  }

  uint32_t GetIndexCount() const {
    return m_indexBuffer ? m_indexBuffer->GetCount() : 0;
  }

private:
  std::vector<std::shared_ptr<VertexBuffer>> m_vertexBuffers;
  std::shared_ptr<IndexBuffer> m_indexBuffer;
};

} // namespace engine