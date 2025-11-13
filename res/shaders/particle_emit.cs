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
uniform uint u_maxParticles;
uniform uint u_emitCount;
uniform float u_emissionRate;

uniform vec3 u_startVelocity;
uniform float u_startLifetime;
uniform vec4 u_startColor;
uniform vec3 u_startSize;

float rand(inout uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return float(seed) / 4294967295.0;
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= u_emitCount) return;
    
    if (u_maxParticles == 0) return;
    
    uint deadIndex = atomicAdd(deadCount, -1);
    if (deadIndex == 0) return;
    
    uint particleIndex = deadIndex - 1;
    uint seed = uint(u_time * 1000.0) + id;
    
    float timeVariation = u_deltaTime * 10.0;
    
    float offsetX = (rand(seed) - 0.5) * 2.0 * (1.0 + timeVariation);
    float offsetZ = (rand(seed) - 0.5) * 2.0 * (1.0 + timeVariation);
    
    float intensity = min(u_emissionRate / 100.0, 1.0);
    
    particles[particleIndex].position = vec4(500, 0.0, 500, u_startLifetime);
    particles[particleIndex].velocity = vec4(u_startVelocity + vec3(offsetX, 0.0, offsetZ) * 0.5 * intensity, u_startLifetime);
    particles[particleIndex].color = u_startColor * intensity;
    particles[particleIndex].size = vec4(u_startSize * intensity, 0.0);
    particles[particleIndex].randomData = uvec4(seed, uint(u_time), 0, 0);
    
    uint aliveIndex = atomicAdd(aliveCount, 1);
}