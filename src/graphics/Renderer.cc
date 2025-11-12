#include "Renderer.h"
#include "graphics/Texture.h"
#include "graphics/VertexArrayObject.h"
#include <exception>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
static float vertices[] = {
    // front face
    -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

    // back face
    -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
    -0.5f};
static unsigned int indices[] = {
    // Front face
    0, 1, 2, 2, 3, 0,

    // Back face
    4, 5, 6, 6, 7, 4,

    // Left face
    0, 3, 7, 7, 4, 0,

    // Right face
    1, 5, 6, 6, 2, 1,

    // Top face
    3, 2, 6, 6, 7, 3,

    // Bottom face
    0, 1, 5, 5, 4, 0};
namespace engine {
void Renderer::Init() {
  m_shader = Shader::Create("res/shaders/shader.sh");
  m_modelShader = Shader::Create("res/shaders/model_shader.sh");
  m_fireGeometryShader = Shader::Create("res/shaders/fire_geom.sh");
  m_instanceModelShader =
      Shader::Create("res/shaders/instance_model_shader.sh");
  m_terrainShader = Shader::Create("res/shaders/terrain.sh");

  m_vao = VertexArrayObject::Create();

  m_vbo = VertexBuffer::Create(vertices, sizeof(vertices));
  m_vbo->SetLayout({{ShaderDataType::Float3, "a_position"}});
  m_vao->AddVertexBuffer(m_vbo);

  m_ebo = IndexBuffer::Create(indices, sizeof(indices) / sizeof(unsigned int));
  m_vao->SetIndexBuffer(m_ebo);

  m_agentVAO = VertexArrayObject::Create();
  m_agentVBO = VertexBuffer::Create(50000 * sizeof(AgentRenderData));
  m_agentVBO->SetLayout({{ShaderDataType::Float3, "a_position"},
                         {ShaderDataType::Float, "a_intensity"},
                         {ShaderDataType::Float3, "a_bounds_min"},
                         {ShaderDataType::Float3, "a_bounds_max"}});
  m_agentVAO->AddVertexBuffer(m_agentVBO);
  m_instanceBuffer = VertexBuffer::Create(10000 * sizeof(glm::mat4));
  m_instanceBuffer->SetLayout(
      {{ShaderDataType::Float4, "a_instance_matrix0", 1},
       {ShaderDataType::Float4, "a_instance_matrix1", 1},
       {ShaderDataType::Float4, "a_instance_matrix2", 1},
       {ShaderDataType::Float4, "a_instance_matrix3", 1}});

  // Создаем отдельный VAO для инстансинга
  m_instanceVAO = VertexArrayObject::Create();

  m_universalParticleShader = Shader::Create("res/shaders/fire_geom.sh");
  
  m_particleVAO = VertexArrayObject::Create();
  m_particleVBO = VertexBuffer::Create(100000 * sizeof(UniversalParticleData));
  m_particleVBO->SetLayout({
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float, "a_intensity"},
      {ShaderDataType::Float3, "a_bounds_min"},
      {ShaderDataType::Float3, "a_bounds_max"},
      {ShaderDataType::Float, "a_effect_type"}
  });
  m_particleVAO->AddVertexBuffer(m_particleVBO);
  SetFireParams({});
}

void Renderer::Shutdown() {
  m_vao.reset();
  m_vbo.reset(); 
  m_ebo.reset();
  m_agentVAO.reset();
  m_agentVBO.reset();
  m_instanceBuffer.reset();
  m_instanceVAO.reset();
  m_particleVAO.reset();
  m_particleVBO.reset();
  
  m_shader.reset();
  m_modelShader.reset();
  m_fireGeometryShader.reset();
  m_instanceModelShader.reset();
  m_terrainShader.reset();
  m_universalParticleShader.reset();
  
  m_fireTexture.reset();
  m_smokeTexture.reset();
  m_sparksTexture.reset();
  m_ashTexture.reset();
  m_heatTexture.reset();
}

void Renderer::DrawCube(const glm::vec3 &position, const glm::vec3 &color,
                        const glm::vec3 &size) {
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  model = glm::scale(model, size);
  m_shader->Bind();

  m_shader->SetMat4("u_model", model);
  m_shader->SetVec3("u_color", color);
  m_shader->SetMat4("u_view", m_viewMat);
  m_shader->SetMat4("u_projection", m_projectionMat);
  m_vao->Bind();
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
  m_vao->Unbind();
  m_shader->Unbind();
}

void Renderer::DrawModel(Model &model, const glm::vec3 &position,
                         const glm::vec3 &scale, const glm::vec3 &camPos) {
  glm::mat4 modelmat = glm::mat4(1.0f);
  modelmat = glm::translate(modelmat, position);
  modelmat = glm::scale(modelmat, scale);
  m_modelShader->Bind();
  m_modelShader->SetMat4("u_model", modelmat);
  m_modelShader->SetMat4("u_view", m_viewMat);
  m_modelShader->SetMat4("u_projection", m_projectionMat);
  m_modelShader->SetVec3("viewPos", camPos);
  m_modelShader->SetVec3("light.position", glm::vec3(500.0f, 200.0f, 100.0f));
  m_modelShader->SetVec3("light.ambient", glm::vec3(0.2f));
  m_modelShader->SetVec3("light.diffuse", glm::vec3(0.8f));
  m_modelShader->SetVec3("light.specular", glm::vec3(1.0f));
  model.Draw(*m_modelShader);
}
void Renderer::DrawModelInstanced(Model &model,
                                  const std::vector<glm::mat4> &instances,
                                  const glm::vec3 &camPos) {
  if (instances.empty())
    return;

  m_instanceModelShader->Bind();
  m_instanceModelShader->SetMat4("u_view", m_viewMat);
  m_instanceModelShader->SetMat4("u_projection", m_projectionMat);
  m_instanceModelShader->SetVec3("viewPos", camPos);
  m_instanceModelShader->SetVec3("light.position",
                                 glm::vec3(500.0f, 200.0f, 100.0f));
  m_instanceModelShader->SetVec3("light.ambient", glm::vec3(0.2f));
  m_instanceModelShader->SetVec3("light.diffuse", glm::vec3(0.8f));
  m_instanceModelShader->SetVec3("light.specular", glm::vec3(1.0f));

  model.DrawInstanced(*m_instanceModelShader, instances);
}

void Renderer::DrawModelWithMaterial(
    Model &model, const glm::vec3 &position, const glm::vec3 &scale,
    const glm::vec3 &camPos, const std::shared_ptr<Material> &material) {

  // 1. Вычисляем матрицу модели
  glm::mat4 modelmat = glm::mat4(1.0f);
  modelmat = glm::translate(modelmat, position);
  modelmat = glm::scale(modelmat, scale);

  // 2. Биндим шейдер и устанавливаем uniform'ы
  m_modelShader->Bind();

  // ДОБАВЬ ПРОВЕРКУ:
  m_modelShader->SetMat4("u_model", modelmat);
  m_modelShader->SetMat4("u_view", m_viewMat);
  m_modelShader->SetMat4("u_projection", m_projectionMat);
  m_modelShader->SetVec3("viewPos", camPos);

  // 3. Настройка освещения (как у тебя было)
  m_modelShader->SetVec3("light.position", glm::vec3(500.0f, 200.0f, 100.0f));
  m_modelShader->SetVec3("light.ambient", glm::vec3(0.2f));
  m_modelShader->SetVec3("light.diffuse", glm::vec3(0.8f));
  m_modelShader->SetVec3("light.specular", glm::vec3(1.0f));

  // 4. Биндим материал (если он есть)
  if (material) {
    material->Bind(*m_modelShader);
  } else {
    m_modelShader->SetBool("material.use_diffuse_map", false);
    m_modelShader->SetBool("material.use_specular_map", false);
    m_modelShader->SetVec3("material.diffuse", glm::vec3(0.8f));
    m_modelShader->SetVec3("material.specular", glm::vec3(0.5f));
    m_modelShader->SetFloat("material.shininess", 32.0f);
  }

  model.DrawGeometryOnly(*m_modelShader);
}

void Renderer::BeginScene(const glm::mat4 &view, const glm::mat4 &projection) {
  glClearColor(0.1, 0.1, 0.1, 1.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  m_viewMat = view;
  m_projectionMat = projection;
}
void Renderer::UploadAgentsData(const std::vector<Agent> &agents) {
  m_particleData.clear();
    
    for (const auto &agent : agents) {
        if (!agent.isBurning()) continue;
        
        float intensity = agent.getOverallIntensity();
        
        // ОГОНЬ (всегда)
        if (m_fireParams.fire.enabled) {
            m_particleData.push_back({
                glm::vec3(agent.position.x, agent.position.y, agent.position.z),
                intensity,
                glm::vec3(agent.fire_collider.bounds_min.x, agent.fire_collider.bounds_min.y, agent.fire_collider.bounds_min.z),
                glm::vec3(agent.fire_collider.bounds_max.x, agent.fire_collider.bounds_max.y, agent.fire_collider.bounds_max.z),
                0.0f // fire
            });
        }
            if (m_fireParams.smoke.enabled && intensity > 0.3f) {
            m_particleData.push_back({
                glm::vec3(agent.position.x, agent.position.y, agent.position.z),
                intensity * 0.7f,
                glm::vec3(agent.fire_collider.bounds_min.x, agent.fire_collider.bounds_min.y, agent.fire_collider.bounds_min.z),
                glm::vec3(agent.fire_collider.bounds_max.x, agent.fire_collider.bounds_max.y, agent.fire_collider.bounds_max.z),
                1.0f // smoke
            });
        }
        
        // ИСКРЫ (при высокой интенсивности)
        if (m_fireParams.sparks.enabled && intensity > 0.7f) {
            m_particleData.push_back({
                glm::vec3(agent.position.x, agent.position.y, agent.position.z),
                intensity * 0.5f,
                glm::vec3(agent.fire_collider.bounds_min.x, agent.fire_collider.bounds_min.y, agent.fire_collider.bounds_min.z),
                glm::vec3(agent.fire_collider.bounds_max.x, agent.fire_collider.bounds_max.y, agent.fire_collider.bounds_max.z),
                2.0f // sparks
            });
        }
          if (m_fireParams.ash.enabled) {
            m_particleData.push_back({
                glm::vec3(agent.position.x, agent.position.y, agent.position.z),
                intensity * 0.3f,
                glm::vec3(agent.fire_collider.bounds_min.x, agent.fire_collider.bounds_min.y, agent.fire_collider.bounds_min.z),
                glm::vec3(agent.fire_collider.bounds_max.x, agent.fire_collider.bounds_max.y, agent.fire_collider.bounds_max.z),
                3.0f // ash
            });
        }
        
        // ТЕПЛО (при любой интенсивности)
        if (m_fireParams.heat.enabled) {
            m_particleData.push_back({
                glm::vec3(agent.position.x, agent.position.y, agent.position.z),
                intensity * 0.2f,
                glm::vec3(agent.fire_collider.bounds_min.x, agent.fire_collider.bounds_min.y, agent.fire_collider.bounds_min.z),
                glm::vec3(agent.fire_collider.bounds_max.x, agent.fire_collider.bounds_max.y, agent.fire_collider.bounds_max.z),
                4.0f // heat
            });
        }
    }
    
    // ОДИН раз загружаем все данные
    if (!m_particleData.empty()) {
        m_particleVBO->Bind();
        m_particleVBO->SetData(m_particleData.data(), m_particleData.size() * sizeof(UniversalParticleData));
    }
}

void Renderer::EndScene() {
  glBindVertexArray(0);
  glUseProgram(0);
}

void Renderer::RenderWithGeometryShader(const glm::vec3 &wind_force) {
   if (m_particleData.empty()) return;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // ← ДОБАВЬ ЭТУ СТРОКУ!

    glDepthMask(GL_FALSE);

    m_universalParticleShader->Bind();
    
    // Общие uniform'ы
    m_universalParticleShader->SetMat4("u_view", m_viewMat);
    m_universalParticleShader->SetMat4("u_projection", m_projectionMat);
    m_universalParticleShader->SetFloat("u_time", glfwGetTime());
    m_universalParticleShader->SetVec3("u_wind_force", wind_force);
    
    // Камерные векторы
    glm::mat4 view = m_viewMat;
    glm::vec3 camera_right = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 camera_up = glm::vec3(view[0][1], view[1][1], view[2][1]);
    m_universalParticleShader->SetVec3("u_camera_right", camera_right);
    m_universalParticleShader->SetVec3("u_camera_up", camera_up);
    
    // БИНДИМ ТЕКСТУРЫ (КОТОРЫЕ УЖЕ ЗАГРУЖЕНЫ В SetFireParams)
    m_fireTexture->Bind(0);
    m_smokeTexture->Bind(1);
    m_sparksTexture->Bind(2);
    m_ashTexture->Bind(3);
    m_heatTexture->Bind(4);
    
    m_universalParticleShader->SetInt("u_fire_texture", 0);
    m_universalParticleShader->SetInt("u_smoke_texture", 1);
    m_universalParticleShader->SetInt("u_sparks_texture", 2);
    m_universalParticleShader->SetInt("u_ash_texture", 3);
    m_universalParticleShader->SetInt("u_heat_texture", 4);
    
    // ОДИН вызов отрисовки для всех эффектов
    m_particleVAO->Bind();
    glDrawArrays(GL_POINTS, 0, m_particleData.size());
    m_particleVAO->Unbind();

    m_universalParticleShader->Unbind();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::DrawTerrain(std::shared_ptr<VertexArrayObject> vao,
                           const glm::vec3 &terrainPos) {
  if (!vao || !m_terrainShader)
    return;

  auto ib = vao->GetIndexBuffer();
  uint32_t count = ib ? ib->GetCount() : 0;
  glm::mat4 model = glm::translate(glm::mat4(1.0f), terrainPos);
  m_terrainShader->Bind();
  m_terrainShader->SetMat4("u_View", m_viewMat);
  m_terrainShader->SetMat4("u_Projection", m_projectionMat);
  m_terrainShader->SetMat4("u_Model", model);

  m_terrainShader->SetVec3("u_LightPos", glm::vec3(100.0f, 100.0f, 100.0f));
  m_terrainShader->SetFloat("u_MaxHeight", 80.0f);

  vao->Bind();
  glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void Renderer::SetEffectParams(const std::string &uniformName, const FireParams::EffectParams &params)
{
 m_universalParticleShader->SetFloat(uniformName + ".particleSize", params.particleSize);
    m_universalParticleShader->SetFloat(uniformName + ".sizeVariation", params.sizeVariation);
    m_universalParticleShader->SetFloat(uniformName + ".animationSpeed", params.animationSpeed);
    m_universalParticleShader->SetFloat(uniformName + ".lifetime", params.lifetime);
    m_universalParticleShader->SetFloat(uniformName + ".spawnRate", params.spawnRate);
    m_universalParticleShader->SetFloat(uniformName + ".riseSpeed", params.riseSpeed);
    m_universalParticleShader->SetFloat(uniformName + ".turbulence", params.turbulence);
    m_universalParticleShader->SetFloat(uniformName + ".windInfluence", params.windInfluence);
    m_universalParticleShader->SetFloat(uniformName + ".spread", params.spread);
    m_universalParticleShader->SetVec3(uniformName + ".baseColor", params.baseColor);
    m_universalParticleShader->SetVec3(uniformName + ".endColor", params.endColor);
    m_universalParticleShader->SetFloat(uniformName + ".colorVariation", params.colorVariation);
    
    // Fragment шейдер параметры
    m_universalParticleShader->SetFloat(uniformName + ".redIntensity", params.redIntensity);
    m_universalParticleShader->SetFloat(uniformName + ".yellowReduction", params.yellowReduction);
    m_universalParticleShader->SetFloat(uniformName + ".alphaMultiplier", params.alphaMultiplier);
    m_universalParticleShader->SetFloat(uniformName + ".textureInfluence", params.textureInfluence);
 }

void Renderer::SetFireParams(const FireParams &params) {
    m_fireParams = params;

    // ЗАГРУЖАЕМ ВСЕ ТЕКСТУРЫ ТОЛЬКО ЗДЕСЬ
    try {
        m_fireTexture = Texture::Create(params.fireTexturePath);
    } catch (const std::exception &e) {
        std::cout << "Failed to load fire texture: " << e.what() << std::endl;
        m_fireTexture = Texture::Create("res/textures/fire_texture3.png");
    }
    
    try {
        m_smokeTexture = Texture::Create(params.smokeTexturePath);
    } catch (const std::exception &e) {
        std::cout << "Failed to load smoke texture: " << e.what() << std::endl;
        m_smokeTexture = Texture::Create("res/textures/smoke_texture.png");
    }
    
    try {
        m_sparksTexture = Texture::Create(params.sparksTexturePath);
    } catch (const std::exception &e) {
        std::cout << "Failed to load sparks texture: " << e.what() << std::endl;
        m_sparksTexture = Texture::Create("res/textures/sparks_texture.png");
    }
    
    try {
        m_ashTexture = Texture::Create(params.ashTexturePath);
    } catch (const std::exception &e) {
        std::cout << "Failed to load ash texture: " << e.what() << std::endl;
        m_ashTexture = Texture::Create("res/textures/ash_texture.png");
    }
    
    try {
        m_heatTexture = Texture::Create(params.heatTexturePath);
    } catch (const std::exception &e) {
        std::cout << "Failed to load heat texture: " << e.what() << std::endl;
        m_heatTexture = Texture::Create("res/textures/heat_texture.png");
    }

    // Устанавливаем параметры в шейдер
    if (m_universalParticleShader) {
        m_universalParticleShader->Bind();
        
        // Параметры для каждого эффекта
        SetEffectParams("u_fire_params", m_fireParams.fire);
        SetEffectParams("u_smoke_params", m_fireParams.smoke);
        SetEffectParams("u_sparks_params", m_fireParams.sparks);
        SetEffectParams("u_ash_params", m_fireParams.ash);
        SetEffectParams("u_heat_params", m_fireParams.heat);
        
        m_universalParticleShader->Unbind();
    }
}
} // namespace engine