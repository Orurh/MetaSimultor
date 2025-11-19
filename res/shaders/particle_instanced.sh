#shader vertex
#version 430 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texcoord;

struct InstanceData {
    vec3 worldPosition;
    vec4 particleData; // xyz = offset, w = size
    vec4 color;
};

layout(std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};

uniform mat4 u_view;
uniform mat4 u_projection;
uniform int u_instanceCount;

out vec2 v_texcoord;
out vec4 v_color;

void main() {
    int instanceID = gl_InstanceID;
    if (instanceID >= u_instanceCount) {
        gl_Position = vec4(0.0);
        return;
    }
    
    InstanceData data = instances[instanceID];
    
    vec3 worldPos = data.worldPosition + data.particleData.xyz;
    float size = data.particleData.w;
    
    vec4 viewPos = u_view * vec4(worldPos, 1.0);
    
    vec3 right = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
    vec3 up = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);
    
    vec3 billboardPos = viewPos.xyz + right * a_position.x * size + up * a_position.y * size;
    
    gl_Position = u_projection * vec4(billboardPos, 1.0);
    v_texcoord = a_texcoord;
    v_color = data.color;
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