#include "OpenGLShader.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
namespace engine {
std::unique_ptr<Shader> CreateOpenGLShader(const std::string &filename) {
  return std::make_unique<OpenGLShader>(filename);
}
std::unique_ptr<Shader> CreateOpenGLShader(const std::string &vertexSrc,
                                           const std::string &fragmentSrc) {
  return std::make_unique<OpenGLShader>(vertexSrc, fragmentSrc);
}

std::unique_ptr<Shader> CreateOpenGLShader(const std::string &vertexSrc,
                                           const std::string &fragmentSrc, const std::string &geometrySrc) {
  return std::make_unique<OpenGLShader>(vertexSrc, fragmentSrc, geometrySrc);
}
OpenGLShader::OpenGLShader(const std::string &filename) {
  std::ifstream stream(filename);
  std::string line;
  enum ShaderType { NONE = -1, VERTEX = 0, GEOMETRY = 1, FRAGMENT = 2 }; 
  std::stringstream ss[3];
  ShaderType type = ShaderType::NONE;
  
  while (getline(stream, line)) {
    if (line.find("#shader") != std::string::npos) {
      if (line.find("vertex") != std::string::npos) {
        type = ShaderType::VERTEX;
      } else if (line.find("geometry") != std::string::npos) { // ДОБАВИЛ
        type = ShaderType::GEOMETRY;
      } else if (line.find("fragment") != std::string::npos) {
        type = ShaderType::FRAGMENT;
      }
    } else {
      if (type != ShaderType::NONE) {
        ss[(int)type] << line << '\n';
      }
    }
  }

  m_rendererID = glCreateProgram();
  
  GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, ss[0].str());
  glAttachShader(m_rendererID, vertexShader);
  
  if (!ss[1].str().empty()) {
    GLuint geometryShader = CompileShader(GL_GEOMETRY_SHADER, ss[1].str());
    glAttachShader(m_rendererID, geometryShader);
  }
  
  GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, ss[2].str());
  glAttachShader(m_rendererID, fragmentShader);
  
  glLinkProgram(m_rendererID);
  
  GLint isLinked;
  glGetProgramiv(m_rendererID, GL_LINK_STATUS, &isLinked);
  if (!isLinked) {
    GLint maxLength;
    glGetProgramiv(m_rendererID, GL_INFO_LOG_LENGTH, &maxLength);
    std::string infoLog(maxLength, ' ');
    glGetProgramInfoLog(m_rendererID, maxLength, &maxLength, &infoLog[0]);
    std::cout << "Shader linking failed: " + infoLog << '\n';
  }
}

OpenGLShader::OpenGLShader(const std::string &vertexSrc,
                           const std::string &fragmentSrc) {
  m_rendererID = glCreateProgram();
  GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
  GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
  glAttachShader(m_rendererID, vertexShader);
  glAttachShader(m_rendererID, fragmentShader);
  glLinkProgram(m_rendererID);
}
OpenGLShader::OpenGLShader(const std::string &vertexSrc,
                           const std::string &geometrySrc,
                           const std::string &fragmentSrc) {
  m_rendererID = glCreateProgram();
  
  GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
  GLuint geometryShader = CompileShader(GL_GEOMETRY_SHADER, geometrySrc);
  GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
  
  glAttachShader(m_rendererID, vertexShader);
  glAttachShader(m_rendererID, geometryShader);
  glAttachShader(m_rendererID, fragmentShader);
  
  glLinkProgram(m_rendererID);
  
  // ПРОВЕРКА ОШИБОК
  GLint isLinked;
  glGetProgramiv(m_rendererID, GL_LINK_STATUS, &isLinked);
  if (!isLinked) {
    GLint maxLength;
    glGetProgramiv(m_rendererID, GL_INFO_LOG_LENGTH, &maxLength);
    std::string infoLog(maxLength, ' ');
    glGetProgramInfoLog(m_rendererID, maxLength, &maxLength, &infoLog[0]);
    std::cout << "Shader linking failed: " + infoLog << '\n';
  }
}

OpenGLShader::~OpenGLShader() { glDeleteProgram(m_rendererID); }

void OpenGLShader::Bind() const { glUseProgram(m_rendererID); }

void OpenGLShader::Unbind() const { glUseProgram(0); }

void OpenGLShader::SetMat4(const std::string &name, const glm::mat4 &matrix) {
  auto loc = glGetUniformLocation(m_rendererID, name.c_str());
  if (loc != -1) {
    glUniformMatrix4fv(loc, 1, GL_FALSE, &matrix[0][0]);
  } else {
    std::cout << "Warning: uniform '" + name + "' doesn't exist!\n";
  }
}

void OpenGLShader::SetVec3(const std::string &name, const glm::vec3 &vector) {
  auto loc = glGetUniformLocation(m_rendererID, name.c_str());
  if (loc != -1) {
    glUniform3fv(loc, 1, &vector[0]);
  } else {
    std::cout << "Warning: uniform '" + name + "' doesn't exist!\n";
  }
}

void OpenGLShader::SetVec4(const std::string &name, const glm::vec4 &vector) {
  auto loc = glGetUniformLocation(m_rendererID, name.c_str());
  if (loc != -1) {
    glUniform4fv(loc, 1, &vector[0]);
  } else {
    std::cout << "Warning: uniform '" + name + "' doesn't exist!\n";
  }
}
void OpenGLShader::SetInt(const std::string &name, int value) {
  auto loc = glGetUniformLocation(m_rendererID, name.c_str());
  if (loc != -1) {
    glUniform1i(loc, value);
  } else {
    std::cout << "Warning: uniform '" + name + "' doesn't exist!\n";
  }
}

void OpenGLShader::SetFloat(const std::string &name, float value) {
  auto loc = glGetUniformLocation(m_rendererID, name.c_str());
  if (loc != -1) {
    glUniform1f(loc, value);
  } else {
    std::cout << "Warning: uniform '" + name + "' doesn't exist!\n";
  }
}

GLuint OpenGLShader::CompileShader(GLenum type, const std::string &source) {
  GLuint shader = glCreateShader(type);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);
  GLint isCompiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if (!isCompiled) {
    GLint maxLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    std::string infoLog(maxLength, ' ');
    glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
    glDeleteShader(shader);
    std::cout << "Shader compilation failed: " + infoLog << '\n';
  }
  return shader;
}
} // namespace engine