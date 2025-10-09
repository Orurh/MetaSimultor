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
    m_modelShader = Shader::Create("res/shaders/model_shader.sh");
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

void Renderer::DrawModel(Model &model, const glm::vec3 &position, const glm::vec3& camPos)
{
    glm::mat4 modelmat = glm::mat4(1.0f);
    modelmat = glm::translate(modelmat, position);
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

void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection) {
    glClearColor(0.1, 0.1, 0.1, 1.0f);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); 
    m_viewMat = view;
    m_projectionMat = projection;
}
void Renderer::UpdateFireParticles(float dt, const std::vector<Agent> &burningAgents)
{
    // for (auto it = m_fireParticles.begin(); it != m_fireParticles.end();) {
    //     it->lifetime -= dt;
    //     it->velocity.y += 3.0f * dt; 
    //     it->position += it->velocity * dt;
    //     it->size *= 0.95f;
        
    //     if (it->lifetime <= 0.0f) {
    //         it = m_fireParticles.erase(it);
    //     } else {
    //         ++it;
    //     }
    // }
    
    // for (const auto& agent : burningAgents) {
    //     int numParticles = 1 + static_cast<int>(agent.intensity * 5);
        
    //     for (int i = 0; i < numParticles && m_fireParticles.size() < 1; i++) {
    //         FireParticle particle;
    //         particle.position = glm::vec3(agent.pos_x, agent.pos_y, agent.pos_z);
            
    //         float angle = static_cast<float>(rand()) / RAND_MAX * 6.2832f; 
    //         float speed = 1.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
            
    //         particle.velocity = glm::vec3(
    //             cos(angle) * 0.5f,
    //             speed * 2.0f, 
    //             sin(angle) * 0.5f
    //         );
            
    //         particle.lifetime = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
    //         particle.maxLifetime = particle.lifetime;
    //         particle.size = 0.1f + agent.intensity * 0.3f;
            
    //         m_fireParticles.push_back(particle);
    //     }
    // }
}
void Renderer::RenderFireParticles()
{
    if (m_fireParticles.empty()) return;
    
    m_particleModels.clear();
    m_particleColors.clear();
    
    for (const auto& particle : m_fireParticles) {
        float lifeRatio = particle.lifetime / particle.maxLifetime;
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, particle.position);
        model = glm::scale(model, glm::vec3(particle.size * lifeRatio));
        m_particleModels.push_back(model);
        
        glm::vec3 color;
        if (lifeRatio > 0.7f) {
            color = glm::mix(glm::vec3(1.0f, 1.0f, 0.3f), glm::vec3(1.0f, 0.8f, 0.1f), (lifeRatio - 0.7f) / 0.3f);
        } else if (lifeRatio > 0.3f) {
            color = glm::mix(glm::vec3(1.0f, 0.8f, 0.1f), glm::vec3(1.0f, 0.4f, 0.1f), (lifeRatio - 0.3f) / 0.4f);
        } else {
            color = glm::mix(glm::vec3(1.0f, 0.4f, 0.1f), glm::vec3(0.8f, 0.2f, 0.1f), lifeRatio / 0.3f);
        }
        
        m_particleColors.push_back(color);
    }
    
    std::vector<float> instanceData;
    for (size_t i = 0; i < m_fireParticles.size(); i++) {
        const glm::mat4& model = m_particleModels[i];
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                instanceData.push_back(model[col][row]);
            }
        }
        instanceData.push_back(m_particleColors[i].x);
        instanceData.push_back(m_particleColors[i].y);
        instanceData.push_back(m_particleColors[i].z);
    }
    
    m_instanceVBO->SetData(instanceData.data(), instanceData.size() * sizeof(float));
    
    m_shader->Bind();
    m_shader->SetMat4("u_view", m_viewMat);
    m_shader->SetMat4("u_projection", m_projectionMat);
    
    m_vao->Bind();
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, m_fireParticles.size());
    m_vao->Unbind();
    m_shader->Unbind();
}
void Renderer::RenderFire(const std::vector<Agent> &agents, float dt)
{
    std::vector<Agent> burningAgents;
    for (const auto& agent : agents) {
        if (agent.isBurning()) {
            burningAgents.push_back(agent);
        }
    }
    
    UpdateFireParticles(dt, burningAgents);
    RenderFireParticles();
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

        for (int i = 0; i < agent.fire_collider.point_count; i++) {

             glm::vec3 world_pos = glm::vec3(
                agent.fire_collider.heat_points[i][0],  
                agent.fire_collider.heat_points[i][1],  
                agent.fire_collider.heat_points[i][2]   
            );
         
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, world_pos);
            
            float intensity = agent.fire_collider.intensities[i];
            if(intensity <= 0.1) {
                continue;
            }
            float scale = 1 + intensity * 0.3f; 
            model = glm::scale(model, glm::vec3(scale));
            
            m_modelMatrices.push_back(model);
            
            glm::vec3 color;
            if (intensity > 0.1f) {
                color = glm::vec3(1.0f, 1.0f - intensity * 0.7f, 0.0f);
            } else {
                color = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            m_colors.push_back(color);
        }

    if (m_modelMatrices.empty()) return;

    std::vector<float> instanceData;
    for (size_t i = 0; i < m_modelMatrices.size(); i++) {
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
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, m_modelMatrices.size());
    m_vao->Unbind();
    m_shader->Unbind();
}
}

} // namespace engine