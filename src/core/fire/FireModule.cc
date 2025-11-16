// core/fire/FireModule.cc
#include "FireModule.h"

FireModule::FireModule()
    : m_materials(m_sim.Config()) // начальная копия конфига
{}

void FireModule::Step(AgentManager &manager, float dt, const glm::vec3 &wind) {
  // 1) синхронизируем ветер (можно дёргать из WindConfig снаружи)
  m_sim.SetWind(wind);

  // 2) конфиг мог поменяться из UI -> обновляем библиотеку материалов
  m_materials.SetConfig(m_sim.Config());

  // 3) запускаем CUDA ядро
  m_sim.Step(manager.getDeviceAgents(), manager.getAgentCount(), dt);
}

void FireModule::ApplyMaterialToAgent(Agent &agent,
                                      std::string_view material_id) {
  // На всякий случай держим конфиги в синхроне
  m_materials.SetConfig(m_sim.Config());
  m_materials.ApplyToCollider(agent.fire_collider, material_id);
}
