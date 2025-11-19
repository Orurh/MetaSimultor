// ResourceCache.h
#pragma once
#include "ComputeShader.h"
#include "ParticleSystem/ParticleEffect.h"
#include "Shader.h"
#include "Texture.h"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
using namespace engine;
class ResourceCache {
public:
  static std::shared_ptr<ParticleEffect>
  GetParticleEffect(const std::string &filepath) {
    static std::unordered_map<std::string, std::shared_ptr<ParticleEffect>>
        cache;

    auto it = cache.find(filepath);
    if (it != cache.end()) {
      return it->second;
    }

    auto effect = std::make_shared<ParticleEffect>();
    effect->LoadFromFile(filepath);
    cache[filepath] = effect;
    return effect;

    return nullptr;
  }
  static std::shared_ptr<ComputeShader>
  GetComputeShader(const std::string &path) {
    if (computeShaders.find(path) == computeShaders.end()) {
      computeShaders[path] = ComputeShader::Create(path);
      std::cout << "[Cache] Loaded compute shader: " << path << "\n";
    }
    return computeShaders[path];
  }

  static std::shared_ptr<Shader> GetShader(const std::string &path) {
    if (shaders.find(path) == shaders.end()) {
      shaders[path] = Shader::Create(path);
      std::cout << "[Cache] Loaded shader: " << path << "\n";
    }
    return shaders[path];
  }

  static std::shared_ptr<Texture> GetTexture(const std::string &path) {
    if (textures.find(path) == textures.end()) {
      textures[path] = Texture::Create(path);
      std::cout << "[Cache] Loaded texture: " << path << "\n";
    }
    return textures[path];
  }

  static void Clear() {
    computeShaders.clear();
    shaders.clear();
    textures.clear();
  }

private:
  inline static std::unordered_map<std::string, std::shared_ptr<ComputeShader>>
      computeShaders;
  inline static std::unordered_map<std::string, std::shared_ptr<Shader>>
      shaders;
  inline static std::unordered_map<std::string, std::shared_ptr<Texture>>
      textures;
};