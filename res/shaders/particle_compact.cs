#version 430 core

layout(local_size_x = 64) in;

struct Particle {
    vec4 position;
    vec4 velocity;  
    vec4 color;
    vec4 size;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

layout(std430, binding = 1) buffer CounterBuffer {
    uint aliveCount;
};

void main() {
    uint id = gl_GlobalInvocationID.x;
    
    // Находим мертвые частицы и перемещаем живые на их места
    if (id < aliveCount && particles[id].position.w <= 0.0) {
        // Ищем живую частицу с конца
        for (uint i = aliveCount - 1; i > id; i--) {
            if (particles[i].position.w > 0.0) {
                // Меняем местами мертвую и живую
                Particle temp = particles[id];
                particles[id] = particles[i];
                particles[i] = temp;
                break;
            }
        }
    }
    
    memoryBarrierBuffer();
    barrier();
    
    // Первая нить обновляет счетчик
    if (id == 0) {
        uint newAlive = 0;
        for (uint i = 0; i < aliveCount; i++) {
            if (particles[i].position.w > 0.0) {
                newAlive++;
            }
        }
        aliveCount = newAlive;
    }
}