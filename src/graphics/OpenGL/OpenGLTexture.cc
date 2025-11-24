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
OpenGLTexture::OpenGLTexture(const TextureSpecification &spec) 
  : m_specification(spec), m_width(spec.Width), m_height(spec.Height) {
  
  GLenum internalFormat = 0, dataFormat = 0;
  
  switch (spec.Format) {
    case ImageFormat::RGBA8:
      internalFormat = GL_RGBA8;
      dataFormat = GL_RGBA;
      break;
    case ImageFormat::RGB8:
      internalFormat = GL_RGB8;
      dataFormat = GL_RGB;
      break;
    case ImageFormat::R8:
      internalFormat = GL_R8;
      dataFormat = GL_RED;
      break;
    default:
      throw std::runtime_error("Unsupported texture format in specification");
  }
  
  glGenTextures(1, &m_rendererID);
  glBindTexture(GL_TEXTURE_2D, m_rendererID);
  
  // Создаем текстуру с пустыми данными
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, dataFormat, GL_UNSIGNED_BYTE, nullptr);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  if (spec.GenerateMips) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }
}


void OpenGLTexture::SetData(const void* data, uint32_t size) {
  GLenum dataFormat = GL_RGBA;
  GLenum internalFormat = GL_RGBA8;

  // Определяем формат данных на основе спецификации текстуры
  switch (m_specification.Format) {
    case ImageFormat::RGBA8:
      internalFormat = GL_RGBA8;
      dataFormat = GL_RGBA;
      break;
    case ImageFormat::RGB8:
      internalFormat = GL_RGB8;
      dataFormat = GL_RGB;
      break;
    case ImageFormat::R8:
      internalFormat = GL_R8;
      dataFormat = GL_RED;
      break;
    default:
      throw std::runtime_error("Unsupported texture format for SetData");
  }

  // Проверяем, что размер данных соответствует ожидаемому
  uint32_t bpp = 4; // байт на пиксель для RGBA
  if (m_specification.Format == ImageFormat::RGB8) bpp = 3;
  else if (m_specification.Format == ImageFormat::R8) bpp = 1;
  
  uint32_t expectedSize = m_width * m_height * bpp;
  if (size != expectedSize) {
    throw std::runtime_error("Texture data size does not match texture dimensions");
  }

  glBindTexture(GL_TEXTURE_2D, m_rendererID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, dataFormat, GL_UNSIGNED_BYTE, data);
  
  if (m_specification.GenerateMips) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }
}



OpenGLTexture::~OpenGLTexture() { glDeleteTextures(1, &m_rendererID); }
void OpenGLTexture::Bind(uint32_t slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, m_rendererID);
}
void OpenGLTexture::Unbind() const {}
} // namespace nova