#include "OpenGLTexture.h"
#include <stb_image.h>
#include <stdexcept>
namespace engine {
std::shared_ptr<Texture> CreateOpenGLTexture(const std::string &path) {
  return std::make_shared<OpenGLTexture>(path);
}

std::shared_ptr<Texture> CreateOpenGLTexture(const TextureSpecification &spec) {
  return std::make_shared<OpenGLTexture>(spec);
}

OpenGLTexture::OpenGLTexture(const std::string &path) {
  int height, width, channels;
  stbi_set_flip_vertically_on_load(1);
  stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
  if (data) {

    m_width = width;
    m_height = height;

    GLenum internalFormat = 0, dataFormat = 0;

    if (channels == 4) {
      internalFormat = GL_RGBA8;
      dataFormat = GL_RGBA;
    } else if (channels == 3) {
      internalFormat = GL_RGB8;
      dataFormat = GL_RGB;
    } else {
      stbi_image_free(data);
      throw std::runtime_error("Unsupported image format: " + path);
    }
    glGenTextures(1, &m_rendererID);
    glBindTexture(GL_TEXTURE_2D, m_rendererID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
  } else {
    throw std::runtime_error("Failed to load texture: " + path);
  }
}
OpenGLTexture::OpenGLTexture(const TextureSpecification &spec) {}
OpenGLTexture::~OpenGLTexture() { glDeleteTextures(1, &m_rendererID); }
void OpenGLTexture::Bind(uint32_t slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, m_rendererID);
}
void OpenGLTexture::Unbind() const {}
} // namespace nova