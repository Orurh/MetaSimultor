#shader vertex
#version 430 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texcoord;

struct Particle {
    vec4 position;
    vec4 velocity;  
    vec4 color;
    vec4 size;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

out vec2 v_texcoord;
out vec4 v_color;

uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    uint particleIndex = gl_InstanceID;
    Particle p = particles[particleIndex];
    
    v_color = p.color;
    
    vec3 worldPos = p.position.xyz;
    float size = p.size.x;
    
    vec4 viewPos = u_view * vec4(worldPos, 1.0);
    viewPos.xy += a_position.xy * size;
    
    gl_Position = u_projection * viewPos;
    v_texcoord = a_texcoord;
}

#shader fragment
#version 430 core

in vec2 v_texcoord;
in vec4 v_color;

out vec4 FragColor;

void main() {
    vec2 centered = v_texcoord - 0.5;
    float dist = length(centered);
    
    if (dist > 0.5) {
        discard;
    }
    
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    FragColor = v_color * alpha;
    
    if (FragColor.a < 0.01) {
        discard;
    }
}