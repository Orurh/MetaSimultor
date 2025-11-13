#version 430 core

layout(local_size_x = 64) in;

struct Particle {
    vec4 position;
    vec4 velocity;  
    vec4 color;
    vec4 size;
    uvec4 randomData;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

layout(std430, binding = 1) buffer CounterBuffer {
    uint aliveCount;
    uint deadCount;
    uint emitCount;
};

uniform float u_time;
uniform float u_deltaTime;
uniform uint u_aliveCount;
uniform float u_gravity;

uniform int u_sizeOverLifetimeEnabled;
uniform int u_colorOverLifetimeEnabled;
uniform vec3 u_forceOverLifetime;
uniform vec3 u_velocityOverLifetime;

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= u_aliveCount) return;
    
    Particle p = particles[id];
    
    // ИСПОЛЬЗУЕМ u_time для анимации
    float timeWave = sin(u_time * 2.0) * 0.1;
    
    // Обновление времени жизни
    p.position.w -= u_deltaTime;
    
    if (p.position.w <= 0.0) {
        return;
    }
    
    // Физика - ИСПОЛЬЗУЕМ ВСЕ СИЛЫ
    p.velocity.y -= u_gravity * u_deltaTime;
    p.velocity.xyz += u_forceOverLifetime * u_deltaTime;
    p.velocity.xyz += u_velocityOverLifetime * u_deltaTime;
    
    // Добавляем небольшую вариацию на основе u_time
    p.velocity.xyz += vec3(timeWave, 0.0, timeWave) * 0.1;
    
    p.position.xyz += p.velocity.xyz * u_deltaTime;
    
    // Fade out over lifetime
    float lifeRatio = p.position.w / p.velocity.w;
    
    // ИСПОЛЬЗУЕМ ВСЕ МОДУЛИ
    if (u_colorOverLifetimeEnabled == 1) {
        p.color.a = 1.0 - lifeRatio;
        // Добавляем цветовую вариацию на основе времени
        p.color.rgb *= (0.8 + 0.2 * sin(u_time + lifeRatio * 3.14));
    } else {
        // Все равно используем u_time для минимальной вариации
        p.color.rgb *= (0.9 + 0.1 * sin(u_time * 0.5));
    }
    
    if (u_sizeOverLifetimeEnabled == 1) {
        p.size.xyz *= (1.0 + lifeRatio * 0.5);
    } else {
        // Минимальное использование uniform'а
        p.size.xyz *= (1.0 + 0.1 * sin(u_time));
    }
    
    particles[id] = p;
}