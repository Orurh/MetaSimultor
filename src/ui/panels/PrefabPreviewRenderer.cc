#include "PrefabPreviewRenderer.h"
#include "graphics/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
namespace ui {

PrefabPreviewRenderer::PrefabPreviewRenderer(int w, int h)
    : m_width(w), m_height(h) {
  SetupFBO();
  UpdateCamera();
}

PrefabPreviewRenderer::~PrefabPreviewRenderer() {
  glDeleteFramebuffers(1, &m_fbo);
  glDeleteTextures(1, &m_texture);
  glDeleteRenderbuffers(1, &m_rbo);
}

void PrefabPreviewRenderer::SetupFBO() {
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_texture, 0);

  glGenRenderbuffers(1, &m_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width,
                        m_height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, m_rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "FBO incomplete!\n";

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PrefabPreviewRenderer::Resize(int w, int h) {
  if (w == m_width && h == m_height)
    return;
  m_width = w;
  m_height = h;
  glDeleteTextures(1, &m_texture);
  glDeleteRenderbuffers(1, &m_rbo);
  SetupFBO();
}

void PrefabPreviewRenderer::UpdateCamera() {
  glm::vec3 front(cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)),
                  sin(glm::radians(m_pitch)),
                  sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));
  front = glm::normalize(front);
  glm::vec3 pos = m_target - front * m_distance;

  m_view = glm::lookAt(pos, m_target, glm::vec3(0, 1, 0));
  m_proj = glm::perspective(glm::radians(45.0f), (float)m_width / m_height,
                            0.1f, 100.0f);
}

void PrefabPreviewRenderer::Rotate(float dyaw, float dpitch) {
  m_yaw += dyaw;
  m_pitch = glm::clamp(m_pitch + dpitch, -89.0f, 89.0f);
  UpdateCamera();
}

void PrefabPreviewRenderer::Zoom(float delta) {
  m_distance = glm::clamp(m_distance - delta, m_minDist, m_maxDist);
  UpdateCamera();
}

void PrefabPreviewRenderer::ResetView() {
  m_yaw = -90.0f;
  m_pitch = 0.0f;
  m_distance = 10.0f;
  UpdateCamera();
}

void PrefabPreviewRenderer::Render(const engine::data::AgentPrefab &P) {
  if (!m_model || m_model->GetPath() != P.m_modelPath) {
    m_model = std::make_shared<engine::Model>(P.m_modelPath);
    m_material = engine::Material::Create();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glViewport(0, 0, m_width, m_height);
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  engine::Renderer::Instance().BeginScene(m_view, m_proj);
  engine::Renderer::Instance().DrawModelWithMaterial(
      *m_model, glm::vec3(0, 0, -10), P.m_modelScale, m_target, m_material);

  // Bounds wireframe
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glm::vec3 center = (P.m_bounds_min + P.m_bounds_max) * 0.5f;
  center.z -= 10;
  glm::vec3 size = P.m_bounds_max - P.m_bounds_min;
  engine::Renderer::Instance().DrawCube(center, glm::vec3(1, 0, 0), size);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  engine::Renderer::Instance().EndScene();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace ui