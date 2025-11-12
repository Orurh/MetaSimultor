#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

out vec3 v_Normal;
out vec3 v_FragPos;
out float v_Height;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_MaxHeight;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_Height = a_Position.y;

    gl_Position = u_Projection * u_View * worldPos;
}

#shader fragment
#version 330 core

in vec3 v_Normal;
in vec3 v_FragPos;
in float v_Height;

out vec4 FragColor;

uniform vec3 u_LightPos = vec3(100.0, 200.0, 100.0);
uniform vec3 u_LightColor = vec3(1.0, 1.0, 0.95);
uniform float u_MaxHeight = 80.0;

void main() {
    // --- Освещение ---
    vec3 norm = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor;

    float ambient = 0.4;
    vec3 lighting = ambient + diffuse;

    // --- Цвет по высоте ---
    float h = v_Height / u_MaxHeight;
    vec3 lowColor  = vec3(0.15, 0.15, 0.15);  // темно-серый
    vec3 highColor = vec3(0.45, 0.45, 0.45);  // серый

    vec3 terrainColor = mix(lowColor, highColor, h);

    // --- Финал ---
    vec3 result = lighting * terrainColor;
    FragColor = vec4(result, 1.0);
}