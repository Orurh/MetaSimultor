#include "ComputeShader.h"
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <sstream>

namespace engine {

std::shared_ptr<ComputeShader>
ComputeShader::Create(const std::string &filepath) {
  auto shader = std::make_shared<ComputeShader>();
  if (shader->LoadFromFile(filepath)) {
    return shader;
  }
  return nullptr;
}

std::shared_ptr<ComputeShader>
ComputeShader::CreateFromSource(const std::string &source) {
  auto shader = std::make_shared<ComputeShader>();
  if (shader->Compile(source)) {
    return shader;
  }
  return nullptr;
}

bool ComputeShader::LoadFromFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cout << "Failed to open compute shader file: " << filepath
              << std::endl;
    return false;
  }

  std::stringstream ss;
  ss << file.rdbuf();
  std::string source = ss.str();
  std::cout << "Loading compute shader from: " << filepath << std::endl;
  return Compile(source);
}

bool ComputeShader::Compile(const std::string &source) {
  const char *sourcePtr = source.c_str();

  // Create shader
  uint32_t shaderID = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(shaderID, 1, &sourcePtr, nullptr);
  glCompileShader(shaderID);

  // Check compilation
  int success;
  glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shaderID, 512, nullptr, infoLog);
    std::cout << "COMPUTE SHADER COMPILATION FAILED:\n" << infoLog << std::endl;
    glDeleteShader(shaderID);
    return false;
  }

  // Create program
  m_rendererID = glCreateProgram();
  glAttachShader(m_rendererID, shaderID);
  glLinkProgram(m_rendererID);

  // Check linking
  glGetProgramiv(m_rendererID, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(m_rendererID, 512, nullptr, infoLog);
    std::cout << "COMPUTE SHADER LINKING FAILED:\n" << infoLog << std::endl;
    glDeleteProgram(m_rendererID);
    glDeleteShader(shaderID);
    m_rendererID = 0;
    return false;
  }

  glDeleteShader(shaderID);
  std::cout << "Compute shader compiled successfully, ID: " << m_rendererID
            << std::endl;
  return true;
}

ComputeShader::~ComputeShader() {
  if (m_rendererID) {
    glDeleteProgram(m_rendererID);
  }
}

void ComputeShader::Bind() const { glUseProgram(m_rendererID); }

void ComputeShader::Unbind() const { glUseProgram(0); }

void ComputeShader::Dispatch(uint32_t groupsX, uint32_t groupsY,
                             uint32_t groupsZ) {
  glDispatchCompute(groupsX, groupsY, groupsZ);
}

void ComputeShader::DispatchIndirect(uint32_t indirectBufferID) {
  glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, indirectBufferID);
  glDispatchComputeIndirect(0);
}

void ComputeShader::MemoryBarrier() {
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                  GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}

// Uniform setters - ВСЕ методы должны использовать GetUniformLocation
int ComputeShader::GetUniformLocation(const std::string &name) {
  auto it = m_uniformLocationCache.find(name);
  if (it != m_uniformLocationCache.end()) {
    return it->second;
  }

  int location = glGetUniformLocation(m_rendererID, name.c_str());
  if (location == -1) {
    std::cout << "Warning: uniform '" << name
              << "' not found in shader (ID: " << m_rendererID << ")"
              << std::endl;
  }

  m_uniformLocationCache[name] = location;
  return location;
}

void ComputeShader::SetInt(const std::string &name, int value) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform1i(location, value);
  }
}

void ComputeShader::SetUInt(const std::string &name, uint32_t value) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform1ui(location, value);
  }
}

void ComputeShader::SetFloat(const std::string &name, float value) {
  int location =
      GetUniformLocation(name); // ИСПРАВЛЕНО: используем GetUniformLocation
  if (location != -1) {
    glUniform1f(location, value);
  }
}

void ComputeShader::SetFloat2(const std::string &name, const glm::vec2 &value) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform2f(location, value.x, value.y);
  }
}

void ComputeShader::SetFloat3(const std::string &name, const glm::vec3 &value) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform3f(location, value.x, value.y, value.z);
  }
}

void ComputeShader::SetFloat4(const std::string &name, const glm::vec4 &value) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform4f(location, value.x, value.y, value.z, value.w);
  }
}

void ComputeShader::SetFloatArray(const std::string &name, const float *values,
                                  uint32_t count) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform1fv(location, count, values);
  }
}

void ComputeShader::SetFloat2Array(const std::string &name,
                                   const glm::vec2 *values, uint32_t count) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform2fv(location, count, (const float *)values);
  }
}

void ComputeShader::SetFloat3Array(const std::string &name,
                                   const glm::vec3 *values, uint32_t count) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform3fv(location, count, (const float *)values);
  }
}

void ComputeShader::SetFloat4Array(const std::string &name,
                                   const glm::vec4 *values, uint32_t count) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform4fv(location, count, (const float *)values);
  }
}

void ComputeShader::SetIntArray(const std::string &name, const int *values,
                                uint32_t count) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform1iv(location, count, values);
  }
}
void ComputeShader::SetMat4(const std::string &name, const glm::mat4 &value) {
  int location = GetUniformLocation(name);
  if (location != -1) {
    glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
  }
}

} // namespace engine