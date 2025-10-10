#shader vertex
#version 330 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_intensity;
layout (location = 2) in vec3 a_bounds_min;
layout (location = 3) in vec3 a_bounds_max;

out float v_intensity;
out vec3 v_bounds_min;
out vec3 v_bounds_max;

void main() {
    gl_Position = vec4(a_position, 1.0);
    v_intensity = a_intensity;
    v_bounds_min = a_bounds_min;
    v_bounds_max = a_bounds_max;
}
#shader geometry
#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 96) out;

in float v_intensity[];
in vec3 v_bounds_min[];
in vec3 v_bounds_max[];

uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_time;
uniform vec3 u_camera_right;
uniform vec3 u_camera_up;

out vec2 f_texcoord;
out float f_intensity;
out float f_alpha;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float noise(vec2 p) {
    return rand(p);
}

void create_fire_particle(vec3 position, float size, float intensity, int particle_id) {
    float time_offset = float(particle_id) * 0.8;
    float life_time = mod(u_time * 1.5 + time_offset, 3.0);
    float life_ratio = life_time / 3.0;
    
    position.y += life_time * 2.0;
    float alpha = 1.0 - smoothstep(0.0, 1.0, life_ratio);
    
    float size_reduction = smoothstep(0.6, 1.0, life_ratio); 
    size *= (1.0 - size_reduction * 0.7); 
    
    float movement_reduction = smoothstep(0.4, 1.0, life_ratio);
    
    vec2 noise_pos = position.xz + u_time;
    position.x += (noise(noise_pos) - 0.5) * 0.8 * life_ratio * (1.0 - movement_reduction);
    position.z += (noise(noise_pos.yx) - 0.5) * 0.8 * life_ratio * (1.0 - movement_reduction);
    

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
        f_alpha = alpha * intensity;
        EmitVertex();
    }
    EndPrimitive();
}

void main() {
    float intensity = v_intensity[0];
    if (intensity < 0.1) return;
    
    vec3 bounds_min = v_bounds_min[0];
    vec3 bounds_max = v_bounds_max[0];
    vec3 bounds_center = (bounds_min + bounds_max) * 0.5;
    vec3 bounds_size = bounds_max - bounds_min;
    
    int num_particles = 200 + int(intensity * 100);
    
    for(int i = 0; i < num_particles; i++) {
        vec3 random = vec3(
            rand(bounds_min.xz + float(i)) * 0.8 - 0.25,
            rand(bounds_min.yx + float(i + 100)) * 0.5 + 0.2,
            rand(bounds_min.zy + float(i + 200)) * 0.8 - 0.25 
        );
        
        vec3 particle_pos = bounds_center + (random - 0.5) * bounds_size * 0.5;        
        float size = 2 + intensity * 0.3; 
        
        create_fire_particle(particle_pos, size, intensity, i);
    }
}

#shader fragment
#version 330 core
in vec2 f_texcoord;
in float f_intensity;
in float f_alpha;

out vec4 FragColor;

uniform sampler2D u_fire_texture;

void main() {
    vec4 tex_color = texture(u_fire_texture, f_texcoord);
    
    if (tex_color.a < 0.1) {
        discard;
    }
    
    vec2 centered_uv = (f_texcoord - 0.5) * 2.0;
    float dist = length(centered_uv);
    
    vec3 final_color = tex_color.rgb;
    final_color.r *= (1.0 + f_alpha * 0.8);  // Краснее к концу
    final_color.g *= (1.0 - f_alpha * 0.3);  // Меньше желтого к концу
    
    // final_color.b *= 0.8; 
    
    float shape_mask = 1.0 - smoothstep(0.7, 1.0, dist);
    float final_alpha = tex_color.a * shape_mask * f_alpha * f_intensity * 1.5;
    
    FragColor = vec4(final_color, final_alpha);
}