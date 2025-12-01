#pragma once

#include <glad/glad.h>
#include <glm/vec4.hpp>

#include "graphics/IndexBuffer.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"

namespace engine::graphics {

struct OpenGLPolicy {
  using ShaderType = Shader;
  using VertexArrayType = VertexArrayObject;
  using VertexBufferType = VertexBuffer;
  using IndexBufferType = IndexBuffer;
  using TextureType = Texture;

  static void InitBackend() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
  }

  static void ShutdownBackend() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
  }

  static void Clear(const glm::vec4 &color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  static void EnableBlend() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  static void DisableBlend() { glDisable(GL_BLEND); }

  static void DepthMask(bool enabled) { glDepthMask(enabled ? GL_TRUE : GL_FALSE); }

  static void DrawIndexedTriangles(const VertexArrayObject &vao, std::size_t indexCount) {
    vao.Bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
    vao.Unbind();
  }
};

}  // namespace engine::graphics
