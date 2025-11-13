// TextureCache.h
#pragma once
#include "OpenGL/OpenGLTexture.h"
#include <memory>
#include <unordered_map>

namespace engine {

class TextureCache {
public:
  static TextureCache &Instance();

  std::shared_ptr<Texture> GetOrLoad(const std::string &path);
  void Clear();

private:
  std::unordered_map<std::string, std::weak_ptr<Texture>> m_cache;
};

} // namespace engine