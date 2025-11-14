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
    uint deadCount; 
};
uniform vec3 u_windForce;

uniform float u_deltaTime;
uniform uint u_aliveCount;
uniform float u_gravity;
uniform float u_simulationSpeed;

// Lifetime modules
uniform vec3 u_emitterPosition;

// Velocity over lifetime
uniform int u_velocityOverLifetimeEnabled;
uniform vec3 u_velocityOverLifetimeLinear;
uniform vec3 u_velocityOverLifetimeOrbital;
uniform float u_velocityOverLifetimeSpeedModifier;

// Force over lifetime
uniform int u_forceOverLifetimeEnabled;
uniform vec3 u_forceOverLifetime;

// Limit velocity
uniform int u_limitVelocityOverLifetimeEnabled;
uniform float u_limitVelocity;
uniform float u_limitVelocityDampen;

// Rotation over lifetime
uniform int u_rotationOverLifetimeEnabled;
uniform vec3 u_rotationOverLifetime;

// Color gradient
uniform int u_colorOverLifetimeEnabled;
uniform vec4 u_colorOverLifetimeGradient[4];
uniform float u_colorOverLifetimeTimes[4];

// Size curve
uniform int u_sizeOverLifetimeEnabled;
uniform vec3 u_sizeOverLifetimeCurve[4];
uniform float u_sizeOverLifetimeTimes[4];

vec4 GetColorFromGradient(float t) {
    for (int i = 0; i < 3; i++) {
        if (t >= u_colorOverLifetimeTimes[i] && t <= u_colorOverLifetimeTimes[i+1]) {
            float segmentT = (t - u_colorOverLifetimeTimes[i]) / 
                           (u_colorOverLifetimeTimes[i+1] - u_colorOverLifetimeTimes[i]);
            return mix(u_colorOverLifetimeGradient[i], 
                      u_colorOverLifetimeGradient[i+1], segmentT);
        }
    }
    return u_colorOverLifetimeGradient[3];
}

vec3 GetSizeFromCurve(float t) {
    for (int i = 0; i < 3; i++) {
        if (t >= u_sizeOverLifetimeTimes[i] && t <= u_sizeOverLifetimeTimes[i+1]) {
            float segmentT = (t - u_sizeOverLifetimeTimes[i]) / 
                           (u_sizeOverLifetimeTimes[i+1] - u_sizeOverLifetimeTimes[i]);
            return mix(u_sizeOverLifetimeCurve[i], 
                      u_sizeOverLifetimeCurve[i+1], segmentT);
        }
    }
    return u_sizeOverLifetimeCurve[3];
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= u_aliveCount) return;
    
    Particle p = particles[id];
    
    float deltaTime = u_deltaTime * u_simulationSpeed;
    p.position.w -= deltaTime;
    
    if (p.position.w <= 0.0) {
        p.color.a = 0.0;
        atomicAdd(deadCount, 1); 
    } else {
        // ОСНОВНАЯ ФИЗИКА
        p.velocity.y += u_gravity * deltaTime;
        p.velocity.xyz += u_windForce * deltaTime;
        // FORCE OVER LIFETIME
        if (u_forceOverLifetimeEnabled == 1) {
            p.velocity.xyz += u_forceOverLifetime * deltaTime;
        }
        
        // VELOCITY OVER LIFETIME
        if (u_velocityOverLifetimeEnabled == 1) {
            p.velocity.xyz += u_velocityOverLifetimeLinear * deltaTime;
            p.velocity.xyz *= u_velocityOverLifetimeSpeedModifier;
            
            if (length(u_velocityOverLifetimeOrbital) > 0.0) {
                vec3 toParticle = p.position.xyz - u_emitterPosition;
                vec3 orbitalForce = cross(u_velocityOverLifetimeOrbital, toParticle);
                p.velocity.xyz += orbitalForce * deltaTime;
            }
        }
        
        // LIMIT VELOCITY
        if (u_limitVelocityOverLifetimeEnabled == 1) {
            float currentSpeed = length(p.velocity.xyz);
            if (currentSpeed > u_limitVelocity) {
                p.velocity.xyz = normalize(p.velocity.xyz) * 
                                mix(currentSpeed, u_limitVelocity, u_limitVelocityDampen * deltaTime);
            }
        }
        
        p.position.xyz += p.velocity.xyz * deltaTime;
        
        float lifeRatio = p.position.w / p.velocity.w;
        float inverseLifeRatio = 1.0 - lifeRatio;
        
        // COLOR OVER LIFETIME
        if (u_colorOverLifetimeEnabled == 1) {
            p.color = GetColorFromGradient(inverseLifeRatio);
        }
        
        // SIZE OVER LIFETIME  
        if (u_sizeOverLifetimeEnabled == 1) {
            p.size.xyz = GetSizeFromCurve(inverseLifeRatio);
        }
        
        if (u_rotationOverLifetimeEnabled == 1) {
            p.size.w += u_rotationOverLifetime.z * deltaTime;
        }

    }
    
    particles[id] = p;
}