#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace engine {

class ComputeShader {
public:
  static std::shared_ptr<ComputeShader> Create(const std::string &filepath);
  static std::shared_ptr<ComputeShader>
  CreateFromSource(const std::string &source);

  ~ComputeShader();
  ComputeShader() = default;

  void Bind() const;
  void Unbind() const;

  void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1);
  void DispatchIndirect(uint32_t indirectBufferID);

  // Memory barrier
  void MemoryBarrier();

  // Uniform setters
  void SetInt(const std::string &name, int value);
  void SetUInt(const std::string &name, uint32_t value);
  void SetFloat(const std::string &name, float value);
  void SetFloat2(const std::string &name, const glm::vec2 &value);
  void SetFloat3(const std::string &name, const glm::vec3 &value);
  void SetFloat4(const std::string &name, const glm::vec4 &value);
  void SetFloatArray(const std::string &name, const float *values,
                     uint32_t count);
  void SetFloat2Array(const std::string &name, const glm::vec2 *values,
                      uint32_t count);
  void SetFloat3Array(const std::string &name, const glm::vec3 *values,
                      uint32_t count);
  void SetFloat4Array(const std::string &name, const glm::vec4 *values,
                      uint32_t count);
  void SetIntArray(const std::string &name, const int *values, uint32_t count);
  void SetMat4(const std::string &name, const glm::mat4 &value);
  void SetBool(const std::string &name, bool value) {
    SetInt(name, value ? 1 : 0);
  }
  uint32_t GetID() const { return m_rendererID; }

private:
  bool LoadFromFile(const std::string &filepath);
  bool Compile(const std::string &source);
  int GetUniformLocation(const std::string &name);

  uint32_t m_rendererID = 0;
  std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace engine