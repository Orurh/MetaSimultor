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

uniform float u_deltaTime;
uniform uint u_aliveCount;
uniform float u_gravity;

// Lifetime modules
uniform int u_sizeOverLifetimeEnabled;
uniform int u_colorOverLifetimeEnabled;
uniform vec3 u_emitterPosition; // ДОБАВИЛИ

// Velocity over lifetime
uniform int u_velocityOverLifetimeEnabled;
uniform vec3 u_velocityOverLifetimeLinear;
uniform vec3 u_velocityOverLifetimeOrbital;

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= u_aliveCount) return;
    
    Particle p = particles[id];
    
    p.position.w -= u_deltaTime;
    
    if (p.position.w <= 0.0) {
        p.color.a = 0.0;
    } else {
        // ОСНОВНАЯ ФИЗИКА
        p.velocity.y += u_gravity * u_deltaTime;
        
        // VELOCITY OVER LIFETIME
        if (u_velocityOverLifetimeEnabled == 1) {
            p.velocity.xyz += u_velocityOverLifetimeLinear * u_deltaTime;
            
            // Орбитальное движение (вращение вокруг эмиттера)
            if (length(u_velocityOverLifetimeOrbital) > 0.0) {
                vec3 toParticle = p.position.xyz - u_emitterPosition;
                vec3 orbitalForce = cross(u_velocityOverLifetimeOrbital, toParticle);
                p.velocity.xyz += orbitalForce * u_deltaTime;
            }
        }
        
        p.position.xyz += p.velocity.xyz * u_deltaTime;
        
        float lifeRatio = p.position.w / p.velocity.w;
        
        // COLOR OVER LIFETIME
        if (u_colorOverLifetimeEnabled == 1) {
            p.color.a = lifeRatio;
        }
        
        // SIZE OVER LIFETIME  
        if (u_sizeOverLifetimeEnabled == 1) {
            p.size.xyz *= (0.3 + 0.7 * lifeRatio);
        }
    }
    
    particles[id] = p;
}