#shader vertex
#version 330 core

layout(location = 0) in vec3 a_position; // XZ-плоскость
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    v_uv = a_uv;
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}

#shader fragment
#version 330 core

in vec2 v_uv;

layout(location = 0) out vec4 FragColor;

uniform sampler2D u_tempMap;

void main() {
    vec4 tex = texture(u_tempMap, v_uv);
    // немного прозрачности, чтобы снизу виден ландшафт
    FragColor = vec4(tex.rgb, tex.a * 0.7);
}
