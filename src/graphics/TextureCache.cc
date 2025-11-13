// TextureCache.cpp
#include "TextureCache.h"

namespace engine {

TextureCache &TextureCache::Instance() {
  static TextureCache instance;
  return instance;
}

std::shared_ptr<Texture> TextureCache::GetOrLoad(const std::string &path) {
  if (path.empty())
    return nullptr;

  auto it = m_cache.find(path);
  if (it != m_cache.end()) {
    if (auto texture = it->second.lock()) {
      return texture;
    }
  }

  auto texture = Texture::Create(path);
  m_cache[path] = texture;
  return texture;
}

void TextureCache::Clear() { m_cache.clear(); }

} // namespace engine