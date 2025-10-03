#pragma once
#include "agent.h"

struct curandStateXORWOW;

void SimulationStep(Agent* agents, int count, float dt);