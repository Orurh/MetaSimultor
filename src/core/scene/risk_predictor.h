#pragma once

#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

#include "../agent.h"
#include "../fire/FireSimulationParams.h"

namespace engine {

class RiskPredictor {
public:
    struct RiskAgent {
        glm::vec3 position{0.0f};
        float fuel_density   = 0.0f;
        float moisture       = 0.0f;
        float ignition_temp  = 0.0f;
        float conductivity   = 0.0f;

        bool  will_burn  = false;
        float burn_time  = 0.0f; // время до воспламенения (сек)
    };

    // Основной вход: посчитать прогноз по текущим агентам
    void PredictRisk(const std::vector<Agent>& agents,
                     const glm::vec3&          fire_origin,
                     const glm::vec3&          wind,
                     const FireSimulationParams& params,
                     float simulation_time = 60.0f); // прогноз на simulation_time секунд

    // Доступ к результатам
    const std::vector<RiskAgent>& GetRiskAgents() const { return m_risk_agents; }

    // Риск в произвольной точке (по ближайшему агенту)
    float GetRiskAtPosition(const glm::vec3& position) const;

    // Упрощённый ответ: сгорит ли точка (по ближайшему агенту)
    bool WillPositionBurn(const glm::vec3& position) const {
        return GetRiskAtPosition(position) > 0.5f;
    }

    // По object_id агента можно получить его RiskAgent (или nullptr)
    const RiskAgent* GetRiskForAgentObjectId(int object_id) const;

    void Clear();

    

private:
    void InitializeRiskAgents(const std::vector<Agent>& agents);

    void SimulateRiskPropagation(const glm::vec3& fire_origin,
                                 const glm::vec3& wind,
                                 const FireSimulationParams& params,
                                 float max_time);

    float CalculateSpreadTime(const RiskAgent& source,
                              const RiskAgent& target,
                              float distance,
                              const FireSimulationParams& params) const;

private:
    std::vector<RiskAgent>          m_risk_agents;
    std::unordered_map<int, int>    m_agent_id_to_index; // object_id -> индекс в m_risk_agents
};

} // namespace engine
