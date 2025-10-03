#include "graphics/Shader.h"
#include <glad/glad.h>
namespace engine {
class OpenGLShader final : public Shader {
public:
  OpenGLShader(const std::string &filename);
  OpenGLShader(const std::string &vertexSrc, const std::string &fragmentSrc);
  virtual ~OpenGLShader();

  void Bind() const override;
  void Unbind() const override;

  void SetMat4(const std::string &name, const glm::mat4 &matrix) override;
  void SetVec3(const std::string &name, const glm::vec3 &vector) override;
  void SetVec4(const std::string &name, const glm::vec4 &vector) override;
  void SetInt(const std::string &name, int value) override;
  void SetFloat(const std::string &name, float value) override;

private:
  GLuint CompileShader(GLenum type, const std::string &source);
  uint32_t m_rendererID;
};
} // namespace engine