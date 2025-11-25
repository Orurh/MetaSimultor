#include "risk_predictor.h"

#include <queue>
#include <algorithm>
#include <limits>
#include <cmath>

namespace engine {

void RiskPredictor::Clear() {
    m_risk_agents.clear();
    m_agent_id_to_index.clear();
}

void RiskPredictor::InitializeRiskAgents(const std::vector<Agent>& agents) {
    m_risk_agents.clear();
    m_risk_agents.reserve(agents.size());
    m_agent_id_to_index.clear();
    m_agent_id_to_index.reserve(agents.size());

    for (size_t i = 0; i < agents.size(); ++i) {
        const Agent& ag = agents[i];

        RiskAgent risk_agent;
        // Позицию берём как "позицию агента" (центр объекта)
        risk_agent.position = ag.position.toGlm();

        const FireCollider& c = ag.fire_collider;
        const int point_count = c.point_count;

        float total_fuel      = 0.0f;
        float total_moisture  = 0.0f;

        for (int j = 0; j < point_count; ++j) {
            total_fuel     += c.fuels[j];
            total_moisture += c.moisture[j];
        }

        const float inv_count = (point_count > 0)
                              ? (1.0f / static_cast<float>(point_count))
                              : 1.0f;

        risk_agent.fuel_density  = total_fuel     * inv_count;
        risk_agent.moisture      = total_moisture * inv_count;
        risk_agent.ignition_temp = c.ignition_temp;
        risk_agent.conductivity  = c.conductivity;

        risk_agent.will_burn = false;
        risk_agent.burn_time = std::numeric_limits<float>::max();

        m_risk_agents.push_back(risk_agent);
        // свяжем object_id -> индекс RiskAgent
        m_agent_id_to_index[ag.object_id] = static_cast<int>(i);
    }
}

void RiskPredictor::SimulateRiskPropagation(const glm::vec3& fire_origin,
                                            const glm::vec3& wind,
                                            const FireSimulationParams& params,
                                            float max_time) {
    using BurnEvent = std::pair<float, int>; // (время, индекс агента)

    std::priority_queue<
        BurnEvent,
        std::vector<BurnEvent>,
        std::greater<BurnEvent>
    > burn_queue;

    if (m_risk_agents.empty())
        return;

    // Нормализованный ветер
    glm::vec3 wind_dir(0.0f);
    float wind_len = glm::length(wind);
    if (wind_len > 1e-3f) {
        wind_dir = wind / wind_len;
    }

    // Находим ближайшего агента к точке возгорания
    int   closest_agent = -1;
    float min_distance  = std::numeric_limits<float>::max();

    for (int i = 0; i < static_cast<int>(m_risk_agents.size()); ++i) {
        float d = glm::distance(fire_origin, m_risk_agents[i].position);
        if (d < min_distance) {
            min_distance  = d;
            closest_agent = i;
        }
    }

    if (closest_agent < 0)
        return;

    // Первый агент загорается мгновенно
    RiskAgent& first = m_risk_agents[closest_agent];
    first.will_burn  = true;
    first.burn_time  = 0.0f;
    burn_queue.push({0.0f, closest_agent});

    // Упрощённый перебор: каждый горящий агент пытается поджечь всех соседей
    // (O(N^2), потом можно ускорять spatial grid'ом)
    const float max_base_dist = params.m_base_spread_distance * 2.0f;

    while (!burn_queue.empty()) {
        const BurnEvent ev = burn_queue.top();
        burn_queue.pop();

        const float current_time = ev.first;
        const int   current_idx  = ev.second;

        if (current_time > max_time)
            continue;

        RiskAgent& current = m_risk_agents[current_idx];

        // если по какой-то причине у нас событие, которое позже, чем уже известное —
        // просто скипаем
        if (!current.will_burn || current_time > current.burn_time + 1e-4f)
            continue;

        for (int neighbor_idx = 0; neighbor_idx < static_cast<int>(m_risk_agents.size()); ++neighbor_idx) {
            if (neighbor_idx == current_idx)
                continue;

            RiskAgent& neighbor = m_risk_agents[neighbor_idx];

            // если уже гарантированно загорается раньше, чем мы его "улучшаем" — пропускаем
            if (neighbor.will_burn &&
                neighbor.burn_time <= current_time + 1e-4f) {
                continue;
            }

            const float distance = glm::distance(current.position, neighbor.position);
            if (distance < 1e-4f)
                continue;

            // учёт ветра: направленный множитель по dot(wind_dir, direction)
            glm::vec3 dir = glm::normalize(neighbor.position - current.position);

            float wind_effect = 1.0f;
            if (wind_len > 1e-3f) {
                float align = glm::dot(wind_dir, dir); // [-1..1]
                // m_wind_heat_bonus ~ 0..1 — по сути сколько "бонуса" даёт ветер
                wind_effect += align * params.m_wind_heat_bonus;
                // не даём уйти в отрицательное значение
                if (wind_effect < 0.2f)
                    wind_effect = 0.2f;
            }

            const float effective_distance = distance / wind_effect;

            if (effective_distance > max_base_dist)
                continue; // слишком далеко с учётом ветра

            const float spread_time =
                CalculateSpreadTime(current, neighbor, effective_distance, params);

            const float ignition_time = current_time + spread_time;
            if (ignition_time < neighbor.burn_time) {
                neighbor.burn_time = ignition_time;
                neighbor.will_burn = true;

                if (ignition_time <= max_time) {
                    burn_queue.push({ignition_time, neighbor_idx});
                }
            }
        }
    }
}

float RiskPredictor::CalculateSpreadTime(const RiskAgent& source,
                                         const RiskAgent& target,
                                         float distance,
                                         const FireSimulationParams& params) const {
    // Базовое время по расстоянию (грубая константа – можно подбирать)
    float base_time = distance * 0.5f; // секунда на 2 метра и т.п.

    // Влияние влажности цели: больше влаги → дольше загорается
    float moisture_penalty =
        target.moisture * params.m_humidity_ignition_factor;

    // Влияние проводимости: высокая проводимость → быстрее прогревается
    float conductivity_boost = 1.0f / (0.1f + target.conductivity);

    // Влияние плотности топлива: чем больше топлива, тем устойчивее и жарче огонь,
    // но и нагреть до воспламенения может быть чуть сложнее. Здесь просто
    // добавляем небольшой множитель.
    float fuel_effect = 1.0f + (target.fuel_density / 100.0f);

    float time =
        base_time *
        (1.0f + moisture_penalty) *
        conductivity_boost *
        fuel_effect;

    // safety
    if (time < 0.1f) time = 0.1f;
    return time;
}

void RiskPredictor::PredictRisk(const std::vector<Agent>&      agents,
                                const glm::vec3&               fire_origin,
                                const glm::vec3&               wind,
                                const FireSimulationParams&    params,
                                float                          simulation_time) {
    InitializeRiskAgents(agents);
    if (m_risk_agents.empty())
        return;

    SimulateRiskPropagation(fire_origin, wind, params, simulation_time);
}

float RiskPredictor::GetRiskAtPosition(const glm::vec3& position) const {
    if (m_risk_agents.empty())
        return 0.0f;

    float min_distance = std::numeric_limits<float>::max();
    float risk         = 0.0f;

    for (const RiskAgent& ra : m_risk_agents) {
        float d = glm::distance(position, ra.position);
        if (d < min_distance) {
            min_distance = d;
            // пока делаем бинарно: либо сгорит, либо нет
            risk = ra.will_burn ? 1.0f : 0.0f;
            // в будущем здесь можно использовать burn_time, чтобы
            // градуировать риск по времени (чем позже – тем меньше риск).
        }
    }

    return risk;
}

const RiskPredictor::RiskAgent*
RiskPredictor::GetRiskForAgentObjectId(int object_id) const {
    auto it = m_agent_id_to_index.find(object_id);
    if (it == m_agent_id_to_index.end())
        return nullptr;

    int idx = it->second;
    if (idx < 0 || idx >= static_cast<int>(m_risk_agents.size()))
        return nullptr;

    return &m_risk_agents[static_cast<size_t>(idx)];
}

} // namespace engine
