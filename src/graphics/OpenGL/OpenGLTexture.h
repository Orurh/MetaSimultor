#pragma once
#include "graphics/Texture.h"
#include <glad/glad.h>
namespace engine {
class OpenGLTexture final : public Texture {
public:
  OpenGLTexture(const std::string &path);
  OpenGLTexture(const TextureSpecification &spec);
  virtual ~OpenGLTexture();
  virtual uint32_t GetId() const override { return m_rendererID; }
  virtual void Bind(uint32_t slot = 0) const override;
  virtual void Unbind() const override;

  virtual uint32_t GetWidth() const override { return m_width; }
  virtual uint32_t GetHeight() const override { return m_height; }
  virtual uint32_t GetRendererID() const override { return m_rendererID; }

  virtual const TextureSpecification &GetSpecification() const override {
    return m_specification;
  }

private:
  uint32_t m_width, m_height;
  uint32_t m_rendererID;
  TextureSpecification m_specification;
};
} // namespace engine