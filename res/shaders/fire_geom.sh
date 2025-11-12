#shader vertex
#version 330 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_intensity;
layout (location = 2) in vec3 a_bounds_min;
layout (location = 3) in vec3 a_bounds_max;
layout (location = 4) in float a_effect_type; 

out float v_intensity;
out vec3 v_bounds_min;
out vec3 v_bounds_max;
out float v_effect_type;

void main() {
    gl_Position = vec4(a_position, 1.0);
    v_intensity = a_intensity;
    v_bounds_min = a_bounds_min;
    v_bounds_max = a_bounds_max;
    v_effect_type = a_effect_type;
}
#shader geometry
#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 96) out;

in float v_intensity[];
in vec3 v_bounds_min[];
in vec3 v_bounds_max[];
in float v_effect_type[];

uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_time;
uniform vec3 u_camera_right;
uniform vec3 u_camera_up;
uniform vec3 u_wind_force;

// УНИВЕРСАЛЬНЫЕ ПАРАМЕТРЫ ДЛЯ ВСЕХ ЭФФЕКТОВ
struct EffectParams {
    float particleSize;
    float sizeVariation;
    float animationSpeed;
    float lifetime;
    float spawnRate;
    float riseSpeed;
    float turbulence;
    float windInfluence;
    float spread;
    vec3 baseColor;
    vec3 endColor;
    float colorVariation;
    float redIntensity;
    float yellowReduction;
    float alphaMultiplier;
    float textureInfluence;
};

// Параметры для каждого типа эффекта
uniform EffectParams u_fire_params;
uniform EffectParams u_smoke_params;
uniform EffectParams u_sparks_params;
uniform EffectParams u_ash_params;
uniform EffectParams u_heat_params;

out vec2 f_texcoord;
out float f_intensity;
out float f_alpha;
out float f_life_ratio;
out vec3 f_color;
out float f_effect_type;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float noise(vec2 p) {
    return rand(p);
}

EffectParams get_effect_params(float effect_type) {
    if (effect_type == 0.0) return u_fire_params;
    if (effect_type == 1.0) return u_smoke_params;
    if (effect_type == 2.0) return u_sparks_params;
    if (effect_type == 3.0) return u_ash_params;
    return u_heat_params;
}

void create_particle(vec3 position, float size, float intensity, int particle_id, float effect_type) {
    EffectParams params = get_effect_params(effect_type);
    
    float time_offset = float(particle_id) * 0.8;
    float life_time = mod(u_time * params.animationSpeed + time_offset, params.lifetime);
    f_life_ratio = life_time / params.lifetime;
    
    // ОДНО объявление life_alpha
    float life_alpha = 1.0 - smoothstep(0.0, 1.0, f_life_ratio);
    f_alpha = life_alpha;  // значение по умолчанию
    
    // РАЗНАЯ ФИЗИКА ДЛЯ КАЖДОГО ЭФФЕКТА
    if (effect_type == 0.0) { // ОГОНЬ
        position.y += life_time * params.riseSpeed;
        size *= (1.0 - smoothstep(0.6, 1.0, f_life_ratio) * 0.7);
    }
    else if (effect_type == 1.0) { // ДЫМ
        position.y += life_time * params.riseSpeed * (1.0 - f_life_ratio * 0.5);
        float appear = smoothstep(0.0, 0.3, f_life_ratio);
        float disappear = smoothstep(0.7, 1.0, f_life_ratio);
        f_alpha = (appear * (1.0 - disappear)) * intensity * 0.8;
        size *= (1.0 + f_life_ratio * 2.0);
    }
    else if (effect_type == 2.0) { // ИСКРЫ
        position.y += life_time * params.riseSpeed * (1.0 - f_life_ratio);
        // Искры падают вниз после пика
        if (f_life_ratio > 0.3) {
            position.y -= (f_life_ratio - 0.3) * params.riseSpeed * 2.0;
        }
        f_alpha = 1.0 - smoothstep(0.5, 1.0, f_life_ratio);
    }
    else if (effect_type == 3.0) { // ПЕПЕЛ
        position.y += life_time * params.riseSpeed * 0.3; // Медленно поднимается
        // Пепел начинает падать
        if (f_life_ratio > 0.4) {
            position.y -= (f_life_ratio - 0.4) * params.riseSpeed;
        }
        f_alpha = (1.0 - smoothstep(0.7, 1.0, f_life_ratio)) * 0.6;
    }
    else if (effect_type == 4.0) { // ТЕПЛО
        position.y += life_time * params.riseSpeed * 0.5;
        f_alpha = (1.0 - smoothstep(0.5, 1.0, f_life_ratio)) * 0.3;
    }
    
    // ДВИЖЕНИЕ И ТУРБУЛЕНТНОСТЬ
    vec2 noise_pos = position.xz + u_time;
    vec2 wind_effect = u_wind_force.xz * f_life_ratio * params.windInfluence;
    
    position.x += (noise(noise_pos) - 0.5) * params.turbulence * f_life_ratio + wind_effect.x;
    position.z += (noise(noise_pos.yx) - 0.5) * params.turbulence * f_life_ratio + wind_effect.y;
    
    if (effect_type == 2.0) { // Искры - случайный цвет
        float color_var = rand(vec2(particle_id, u_time));
        f_color = mix(params.baseColor, params.endColor, color_var);
    } else {
        f_color = mix(params.baseColor, params.endColor, life_alpha);
    }
    
    // СОЗДАНИЕ КВАДРА
    vec3 right = u_camera_right * size;
    vec3 up = u_camera_up * size;
    
    vec3 vertices[4] = vec3[4](
        position - right - up,
        position + right - up,
        position - right + up,
        position + right + up
    );
    
    vec2 texcoords[4] = vec2[4](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );
    
    for(int i = 0; i < 4; i++) {
        gl_Position = u_projection * u_view * vec4(vertices[i], 1.0);
        f_texcoord = texcoords[i];
        f_intensity = intensity;
        f_effect_type = effect_type;
        EmitVertex();
    }
    EndPrimitive();
}

void main() {
    float intensity = v_intensity[0];
    float effect_type = v_effect_type[0];
    
    if (intensity < 0.05) return;
    
    EffectParams params = get_effect_params(effect_type);
    
    vec3 bounds_min = v_bounds_min[0];
    vec3 bounds_max = v_bounds_max[0];
    vec3 bounds_center = (bounds_min + bounds_max) * 0.5;
    vec3 bounds_size = bounds_max - bounds_min;
    
    int num_particles = int(params.spawnRate + intensity * 50.0);
    
    for(int i = 0; i < num_particles; i++) {
        vec3 random = vec3(
            rand(bounds_min.xz + float(i)) * params.spread - (params.spread * 0.5),
            rand(bounds_min.yx + float(i + 100)) * params.spread * 0.5,
            rand(bounds_min.zy + float(i + 200)) * params.spread - (params.spread * 0.5)
        );
        
        vec3 particle_pos = bounds_center + (random - 0.5) * bounds_size * 0.5;        
        float size = params.particleSize + intensity * params.sizeVariation;
        
        create_particle(particle_pos, size, intensity, i, effect_type);
    }
}
#shader fragment
#version 330 core
in vec2 f_texcoord;
in float f_intensity;
in float f_alpha;
in float f_life_ratio;
in vec3 f_color;
in float f_effect_type;

out vec4 FragColor;

uniform sampler2D u_fire_texture;
uniform sampler2D u_smoke_texture;
uniform sampler2D u_sparks_texture;
uniform sampler2D u_ash_texture;
uniform sampler2D u_heat_texture;

struct EffectParams {
    float particleSize;
    float sizeVariation;
    float animationSpeed;
    float lifetime;
    float spawnRate;
    float riseSpeed;
    float turbulence;
    float windInfluence;
    float spread;
    vec3 baseColor;
    vec3 endColor;
    float colorVariation;
    float redIntensity;
    float yellowReduction;
    float alphaMultiplier;
    float textureInfluence;
};

uniform EffectParams u_fire_params;
uniform EffectParams u_smoke_params;
uniform EffectParams u_sparks_params;
uniform EffectParams u_ash_params;
uniform EffectParams u_heat_params;

EffectParams get_effect_params(float effect_type) {
    if (effect_type == 0.0) return u_fire_params;
    if (effect_type == 1.0) return u_smoke_params;
    if (effect_type == 2.0) return u_sparks_params;
    if (effect_type == 3.0) return u_ash_params;
    return u_heat_params;
}

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec4 tex_color;
    
    // РАЗНЫЕ ТЕКСТУРЫ ДЛЯ КАЖДОГО ЭФФЕКТА
    if (f_effect_type == 0.0) {
        tex_color = texture(u_fire_texture, f_texcoord);
    }
    else if (f_effect_type == 1.0) {
        tex_color = texture(u_smoke_texture, f_texcoord);
    }
    else if (f_effect_type == 2.0) {
        tex_color = texture(u_sparks_texture, f_texcoord);
    }
    else if (f_effect_type == 3.0) {
        tex_color = texture(u_ash_texture, f_texcoord);
    }
    else if (f_effect_type == 4.0) {
        tex_color = texture(u_heat_texture, f_texcoord);
    }
    
    if (tex_color.a < 0.1) {
        discard;
    }
    
    vec2 centered_uv = (f_texcoord - 0.5) * 2.0;
    float dist = length(centered_uv);
    float shape_mask = 1.0 - smoothstep(0.7, 1.0, dist);
    
    // ПОЛУЧАЕМ ПАРАМЕТРЫ ДЛЯ ТЕКУЩЕГО ЭФФЕКТА
    EffectParams params = get_effect_params(f_effect_type);
    
    // ТЕСТ: используем ТОЛЬКО цвет из текстуры для проверки
    vec3 final_color = mix(f_color, tex_color.rgb, params.textureInfluence);
    
    // Если текстура отображается правильно, можно раскомментировать миксинг:
    // vec3 final_color = mix(f_color, tex_color.rgb, params.textureInfluence);
    
    // ПРИМЕНЯЕМ ЦВЕТОВЫЕ ЭФФЕКТЫ
    final_color.r *= (1.0 + f_alpha * params.redIntensity);
    final_color.g *= (1.0 - f_alpha * params.yellowReduction);
    
    // ЦВЕТОВАЯ ВАРИАЦИЯ
    if (params.colorVariation > 0.0) {
        float variation = rand(f_texcoord + f_life_ratio) * params.colorVariation;
        final_color = mix(final_color, final_color * (0.8 + variation * 0.4), params.colorVariation);
    }
    
    // ФИНАЛЬНАЯ ПРОЗРАЧНОСТЬ
    float final_alpha = tex_color.a * shape_mask * f_alpha * f_intensity * params.alphaMultiplier;
    
    // ДОПОЛНИТЕЛЬНЫЕ ЭФФЕКТЫ ДЛЯ КОНКРЕТНЫХ ТИПОВ
    if (f_effect_type == 1.0) { // ДЫМ
        float edge_alpha = 1.0 - smoothstep(0.3, 0.8, dist);
        final_alpha *= edge_alpha;
    }
    else if (f_effect_type == 2.0) { // ИСКРЫ
        float flicker = sin(f_life_ratio * 30.0) * 0.3 + 0.7;
        final_alpha *= flicker;
    }
    else if (f_effect_type == 4.0) { // ТЕПЛО
        shape_mask = 1.0 - smoothstep(0.3, 0.9, dist);
        final_alpha *= shape_mask;
    }
    
    FragColor = vec4(final_color, final_alpha);
}