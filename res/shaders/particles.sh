#shader vertex
#version 430 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texcoord;

struct Particle {
    vec4 position;
    vec4 velocity;  
    vec4 color;
    vec4 size;        // xyz = размер, w = rotation
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
    v_texcoord = a_texcoord;
    
    vec3 worldPos = p.position.xyz;
    float size = p.size.x;
    float rotation = p.size.w; 
    
    vec2 pos = a_position.xy * size;
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    vec2 rotatedPos = vec2(
        pos.x * cosR - pos.y * sinR,
        pos.x * sinR + pos.y * cosR
    );
    
    vec4 viewPos = u_view * vec4(worldPos, 1.0);
    viewPos.xy += rotatedPos;
    
    gl_Position = u_projection * viewPos;
}
#shader fragment  
#version 430 core

in vec2 v_texcoord;
in vec4 v_color;

out vec4 FragColor;

uniform sampler2D u_particleTexture;
uniform int u_useTexture;

void main() {
    if (u_useTexture == 1) {
        FragColor = texture(u_particleTexture, v_texcoord) * v_color;
    } else {
        // Сплошной квадрат без скругления
        FragColor = v_color;
    }
}