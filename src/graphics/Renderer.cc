#include "Renderer.h"
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
static float vertices[] = {
      // front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        
        // back face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
};
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
    0, 1, 5, 5, 4, 0
};
namespace engine {
void Renderer::Init() {
 m_shader = Shader::Create("res/shaders/shader.sh");
    m_vao = VertexArrayObject::Create();

    m_vbo = VertexBuffer::Create(vertices, sizeof(vertices));
    m_vbo->SetLayout({{ShaderDataType::Float3, "a_position"}});
    m_vao->AddVertexBuffer(m_vbo);

    m_instanceVBO = VertexBuffer::Create(100000 * (sizeof(glm::mat4) + sizeof(glm::vec3)));
    m_instanceVBO->SetLayout({
        {ShaderDataType::Float4, "a_instance_model_0", 1},
        {ShaderDataType::Float4, "a_instance_model_1", 1},
        {ShaderDataType::Float4, "a_instance_model_2", 1},
        {ShaderDataType::Float4, "a_instance_model_3", 1},
        {ShaderDataType::Float3, "a_instance_color", 1}
    });
    m_vao->AddVertexBuffer(m_instanceVBO);

    m_ebo = IndexBuffer::Create(indices, sizeof(indices) / sizeof(unsigned int));
    m_vao->SetIndexBuffer(m_ebo);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
}

void Renderer::Shutdown()
{
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& color, float scale) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(scale));
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

void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection) {
    glClearColor(0.1, 0.1, 0.1, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_viewMat = view;
    m_projectionMat = projection;
}
void Renderer::EndScene()
{
}
void Renderer::DrawAgents(const std::vector<Agent> &agents)
{
   if (agents.empty()) return;

    m_modelMatrices.clear();
    m_colors.clear();
    for (const auto& agent : agents) {
      // Матрица модели для каждого агента
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(agent.pos_x, 0.0f, agent.pos_y));
      
      float scale = (agent.type == 0) ? 8.0f : (agent.type == 1) ? 4.0f : 6.0f;
      model = glm::scale(model, glm::vec3(scale));
      
      m_modelMatrices.push_back(model);
      
      // Цвет для каждого агента
      glm::vec3 color;
      if (agent.type == 0) color = glm::vec3(0.0f, 1.0f, 0.0f);
      else if (agent.type == 1) color = glm::vec3(1.0f, agent.intensity, 0.0f);
      else color = glm::vec3(0.3f, 0.3f, 0.3f);
      
      m_colors.push_back(color);
    }

    std::vector<float> instanceData;
    for (size_t i = 0; i < agents.size(); i++) {
        const glm::mat4& model = m_modelMatrices[i];
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                instanceData.push_back(model[col][row]);
            }
        }
        instanceData.push_back(m_colors[i].x);
        instanceData.push_back(m_colors[i].y);
        instanceData.push_back(m_colors[i].z);
    }

    m_instanceVBO->SetData(instanceData.data(), instanceData.size() * sizeof(float));

    m_shader->Bind();
    m_shader->SetMat4("u_view", m_viewMat);
    m_shader->SetMat4("u_projection", m_projectionMat);
    
    m_vao->Bind();
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, agents.size());
    m_vao->Unbind();
    m_shader->Unbind();
}

} // namespace engine