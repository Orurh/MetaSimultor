#version 430 core
layout(local_size_x = 64) in;

struct Particle {
    vec4 position;
    vec4 velocity;  
    vec4 color;
    vec4 size;
};

struct GridCell {
    float temperature;
    float intensity;
    float smoke_density;
    float padding;
    vec3 wind;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

layout(std430, binding = 1) buffer CounterBuffer {
    uint aliveCount;
    uint deadCount;
};

layout(std430, binding = 2) buffer GridBuffer {
    GridCell grid[];
};

// === EMIT UNIFORMS ===
uniform uint u_emitCount;
uniform uint u_maxParticles;
uniform vec3 u_emitterPosition;
uniform vec3 u_startVelocity;
uniform float u_startLifetime;
uniform vec4 u_startColor;
uniform vec3 u_startSize;
uniform vec3 u_startRotation;
uniform int u_use3DRotation;

uniform int u_shapeType;
uniform float u_shapeRadius;
uniform float u_shapeAngle;
uniform vec3 u_shapeBoxSize;
uniform float u_shapeArc;
uniform float u_shapeRandomDirection;
uniform int u_shapeAlignToDirection;

// === UPDATE UNIFORMS ===
uniform float u_deltaTime;
uniform uint u_aliveCount;
uniform float u_simulationSpeed;
uniform vec3 u_windForce;
uniform float u_gravity;

// Lifetime modules
uniform int u_sizeOverLifetimeEnabled;
uniform int u_colorOverLifetimeEnabled;

// Color over lifetime gradient
uniform vec4 u_colorOverLifetimeGradient[4];
uniform float u_colorOverLifetimeTimes[4];

// Size over lifetime curve
uniform vec3 u_sizeOverLifetimeCurve[4];
uniform float u_sizeOverLifetimeTimes[4];

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

// === CUDA UNIFORMS ===
uniform int u_grid_size_x;
uniform int u_grid_size_z;
uniform float u_cell_size;
uniform vec3 u_world_origin;

// Random function
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
    uint globalIndex = gl_GlobalInvocationID.x;
    uint totalCells = u_grid_size_x * u_grid_size_z;
    
    // 🔥 EMIT: используем CUDA данные для определения где спавнить
    if (globalIndex < u_emitCount) {
        // Распределяем частицы по ячейкам с огнем
        uint cell_index = globalIndex % totalCells;
        GridCell cuda_cell = grid[cell_index];
        
        // 🎯 СПАВНИМ ТОЛЬКО В ЯЧЕЙКАХ С ДОСТАТОЧНОЙ ТЕМПЕРАТУРОЙ
        if (cuda_cell.temperature > 150.0 && cuda_cell.intensity > 0.2) {
            uint particleIndex = atomicAdd(aliveCount, 1);
            if (particleIndex >= u_maxParticles) {
                atomicAdd(aliveCount, -1);
                return;
            }
            
            uint seed = uint(globalIndex * 7919 + cell_index * 137);
            
            // 🎯 ПОЗИЦИЯ В МИРЕ ИЗ CUDA СИСТЕМЫ
            vec3 cell_world_pos = u_world_origin + vec3(
                float(cell_index % u_grid_size_x) * u_cell_size,
                0.0,
                float(cell_index / u_grid_size_x) * u_cell_size
            );
            
            // 🔥 ФОРМА ИЗ ТВОИХ НАСТРОЕК
            vec3 spawn_offset = GetSpawnPosition(seed, u_shapeType);
            vec3 world_pos = cell_world_pos + spawn_offset;
            
            // 🎯 МОДИФИКАЦИИ НА ОСНОВЕ CUDA ДАННЫХ
            float temp_factor = clamp(cuda_cell.temperature / 800.0, 0.0, 1.0);
            float intensity_factor = clamp(cuda_cell.intensity, 0.0, 1.0);
            
            // Цвет становится ярче/желтее при высокой температуре
            vec4 particle_color = u_startColor;
            particle_color.rgb = mix(particle_color.rgb, vec3(1.0, 0.9, 0.3), temp_factor * 0.7);
            particle_color.a *= intensity_factor;
            
            // Размер увеличивается с температурой
            vec3 particle_size = u_startSize * (1.0 + temp_factor * 0.5);
            
            // Скорость зависит от интенсивности
            vec3 particle_velocity = u_startVelocity * (1.0 + intensity_factor * 0.8);
            
            // Ветер из CUDA системы
            particle_velocity += cuda_cell.wind * intensity_factor;
            
            float lifetime_variation = 0.7 + 0.6 * rand(seed);
            float actual_lifetime = u_startLifetime * lifetime_variation;
            
            particles[particleIndex].position = vec4(world_pos, actual_lifetime);
            particles[particleIndex].velocity = vec4(particle_velocity, actual_lifetime);
            particles[particleIndex].color = particle_color;
            
            if (u_use3DRotation == 1) {
                particles[particleIndex].size = vec4(particle_size, u_startRotation.x);
            } else {
                particles[particleIndex].size = vec4(particle_size.x, 0.0, 0.0, 0.0);
            }
        }
    }
    
    // 🔥 UPDATE: твоя полная логика обновления
    barrier();
    
    if (globalIndex < aliveCount) {
        Particle p = particles[globalIndex];
        
        if (p.position.w <= 0.0) {
            p.color.a = 0.0;
            atomicAdd(deadCount, 1);
        } else {
            float deltaTime = u_deltaTime * u_simulationSpeed;
            p.position.w -= deltaTime;
            
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
        
        particles[globalIndex] = p;
    }
}