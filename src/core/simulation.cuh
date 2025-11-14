#pragma once
#include "agent.h"
#include <cuda_runtime.h>
#include <glm/glm.hpp>
struct curandStateXORWOW;
struct HashEntry {
    int agent_index;
    int next;
};

struct HashTable {
    int* table;        // [table_size]
    HashEntry* entries; // [max_agents]
    int entry_count;
    int table_size;
};
void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force);