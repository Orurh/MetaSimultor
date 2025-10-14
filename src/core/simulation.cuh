#pragma once
#include "agent.h"
#include <cuda_runtime.h>
#include <glm/glm.hpp>
struct curandStateXORWOW;

void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force);