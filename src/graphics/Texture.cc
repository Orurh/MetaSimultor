#include "Texture.h"

namespace engine {
std::shared_ptr<Texture> CreateOpenGLTexture(const std::string &path);
std::shared_ptr<Texture> CreateOpenGLTexture(const TextureSpecification &spec);
} // namespace engine

namespace engine {
std::shared_ptr<Texture> Texture::Create(const std::string &path) {
  return CreateOpenGLTexture(path);
}

std::shared_ptr<Texture> Texture::Create(const TextureSpecification &spec) {
  return CreateOpenGLTexture(spec);
}
} // namespace engine