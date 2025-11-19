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

// === EMIT UNIFORMS ===
uniform uint u_maxParticles;  
uniform uint u_emitCount;
uniform vec3 u_emitterPosition;
uniform vec3 u_startVelocity;
uniform float u_startLifetime;
uniform vec4 u_startColor;
uniform vec3 u_startSize;

uniform int u_shapeType;
uniform float u_shapeRadius;
uniform float u_shapeAngle;
uniform vec3 u_shapeBoxSize;
uniform float u_shapeArc;
uniform float u_shapeRandomDirection;
uniform bool u_shapeAlignToDirection;

uniform vec3 u_startRotation;
uniform bool u_use3DRotation;

// === UPDATE UNIFORMS ===
uniform float u_deltaTime;
uniform uint u_aliveCount;
uniform float u_gravity;
uniform float u_simulationSpeed;
uniform vec3 u_windForce;

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

// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===
float rand(inout uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return float(seed) / 4294967295.0;
}

vec3 GetSpawnPosition(inout uint seed, int shapeType) {
    if (shapeType == 1) { // Cone
        float angleRad = radians(u_shapeAngle);
        float distance = u_shapeRadius * rand(seed);
        float arcRad = radians(u_shapeArc);
        float randomAngle = rand(seed) * arcRad;
        
        return vec3(
            cos(randomAngle) * distance * tan(angleRad),
            sin(randomAngle) * distance * tan(angleRad),
            -distance
        );
    } else if (shapeType == 2) { // Box
        return vec3(
            (rand(seed) - 0.5) * u_shapeBoxSize.x,
            (rand(seed) - 0.5) * u_shapeBoxSize.y, 
            (rand(seed) - 0.5) * u_shapeBoxSize.z
        );
    } else { // Sphere (default)
        float theta = rand(seed) * radians(u_shapeArc);
        float phi = acos(1.0 - 2.0 * rand(seed));
        float r = u_shapeRadius * pow(rand(seed), 1.0/3.0);
        
        return vec3(
            r * sin(phi) * cos(theta),
            r * sin(phi) * sin(theta), 
            r * cos(phi)
        );
    }
}

vec3 ApplyRandomDirection(inout uint seed, vec3 baseDirection, vec3 spawnPosition) {
    if (u_shapeRandomDirection > 0.0) {
        float randomAngle = rand(seed) * radians(360.0) * u_shapeRandomDirection;
        float randomStrength = rand(seed) * u_shapeRandomDirection;
        
        vec3 randomOffset = vec3(
            cos(randomAngle) * randomStrength,
            sin(randomAngle) * randomStrength,
            (rand(seed) - 0.5) * randomStrength * 2.0
        );
        
        return normalize(baseDirection + randomOffset);
    }
    return baseDirection;
}

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

// === EMIT ФУНКЦИЯ ===
void emitParticle(uint id) {
    if (id >= u_emitCount) return;
    
    uint index = atomicAdd(aliveCount, 1) % u_maxParticles; 
    uint seed = uint(index * 7919 + id * 137);
    
    vec3 spawnOffset = GetSpawnPosition(seed, u_shapeType);
    
    vec3 initialVelocity = u_startVelocity;
    if (u_shapeAlignToDirection && length(spawnOffset) > 0.001) {
        initialVelocity = normalize(spawnOffset) * length(u_startVelocity);
    }
    
    initialVelocity = ApplyRandomDirection(seed, initialVelocity, spawnOffset);
    
    float sizeVariation = 0.5 + 0.5 * rand(seed);
    float lifetimeVariation = 0.7 + 0.6 * rand(seed);
    float speedVariation = 0.8 + 0.4 * rand(seed);
    
    vec4 startColor = u_startColor;
    startColor.rgb *= (0.9 + 0.2 * rand(seed));
    
    particles[index].position = vec4(
        u_emitterPosition + spawnOffset,
        u_startLifetime * lifetimeVariation
    );
    
    particles[index].velocity = vec4(
        initialVelocity * speedVariation,
        u_startLifetime * lifetimeVariation
    );
    
    particles[index].color = startColor;
    
    if (u_use3DRotation) {
        particles[index].size = vec4(u_startSize * sizeVariation, 0.0);
    } else {
        particles[index].size = vec4(u_startSize.x * sizeVariation, 0.0, 0.0, 0.0);
    }
}

// === UPDATE ФУНКЦИЯ ===
void updateParticle(uint id) {
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

// === ОСНОВНАЯ ФУНКЦИЯ ===
void main() {
    uint globalIndex = gl_GlobalInvocationID.x;
    uint totalWorkItems = gl_NumWorkGroups.x * gl_WorkGroupSize.x;
    
    // EMIT: обрабатываем новые частицы
    if (globalIndex < u_emitCount) {
        emitParticle(globalIndex);
    }
    
    // UPDATE: обрабатываем существующие частицы
    // Используем барьер чтобы emit завершился перед update
    barrier();
    memoryBarrierBuffer();
    
    if (globalIndex < u_aliveCount) {
        updateParticle(globalIndex);
    }
}