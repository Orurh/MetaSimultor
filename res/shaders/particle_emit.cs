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
uniform uint u_maxParticles;  
uniform uint u_emitCount;
uniform vec3 u_emitterPosition;
uniform vec3 u_startVelocity;
uniform float u_startLifetime;
uniform vec4 u_startColor;
uniform vec3 u_startSize;

// Shape module uniforms
uniform int u_shapeType;
uniform float u_shapeRadius;
uniform float u_shapeAngle;
uniform vec3 u_shapeBoxSize;
uniform float u_shapeArc;
uniform float u_shapeRandomDirection;
uniform bool u_shapeAlignToDirection;

// Rotation
uniform vec3 u_startRotation;
uniform bool u_use3DRotation;

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
        
        // Создаем случайное отклонение
        vec3 randomOffset = vec3(
            cos(randomAngle) * randomStrength,
            sin(randomAngle) * randomStrength,
            (rand(seed) - 0.5) * randomStrength * 2.0
        );
        
        return normalize(baseDirection + randomOffset);
    }
    return baseDirection;
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= u_emitCount) return;
    uint index = atomicAdd(aliveCount, 1) % 50000; 
    uint seed = uint(index * 7919 + id * 137);
    
    vec3 spawnOffset = GetSpawnPosition(seed, u_shapeType);
    
    vec3 initialVelocity = u_startVelocity;
    if (u_shapeAlignToDirection && length(spawnOffset) > 0.001) {
        initialVelocity = normalize(spawnOffset) * length(u_startVelocity);
    }
    
    initialVelocity = ApplyRandomDirection(seed, initialVelocity, spawnOffset);
    
    // Случайные вариации
    float sizeVariation = 0.5 + 0.5 * rand(seed);
    float lifetimeVariation = 0.7 + 0.6 * rand(seed);
    float speedVariation = 0.8 + 0.4 * rand(seed);
    
    // Начальный цвет с вариацией
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
    
    // Размер с учетом 3D
    if (u_use3DRotation) {
        particles[index].size = vec4(u_startSize * sizeVariation, 0.0);
    } else {
        particles[index].size = vec4(u_startSize.x * sizeVariation, 0.0, 0.0, 0.0);
    }
}