#include "Scene.h"
#include "Entity.h"
#include "core/simulation.cuh"
#include <cuda_runtime.h>
#include <iostream>
namespace engine {
Scene::Scene() : m_agentManager(1000000)
{
}

void Scene::Init()
{
    m_renderer.Init();
    for(int i = 0; i < 200; i+=5){
        for(int j = 0; j < 200; j+=5) {
            AddTree(500 + i, 500 + j);
        }
    }

    AddFire(510, 500);
}

void Scene::SetViewProjection(const glm::mat4& view, const glm::mat4& projection) {
    m_view = view;
    m_projection = projection;
}

void Scene::Update(float dt) {
    SimulationStep(m_agentManager.getDeviceAgents(), m_agentManager.getAgentCount(), dt);
}

void Scene::Render() {
     m_renderer.BeginScene(m_view, m_projection);
   
    std::vector<Agent> agents(m_agentManager.getAgentCount());
    cudaMemcpy(agents.data(), m_agentManager.getDeviceAgents(),
               agents.size() * sizeof(Agent), cudaMemcpyDeviceToHost);
    m_renderer.DrawAgents(agents);
    
    m_renderer.EndScene();
}

}
