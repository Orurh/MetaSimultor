#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

out vec3 v_Normal;
out vec3 v_FragPos;
out vec2 v_TexCoord;
out float v_Height;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_MaxHeight;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_TexCoord = a_TexCoord;
    v_Height = a_Position.y;

    gl_Position = u_Projection * u_View * worldPos;
}
#shader fragment
#version 330 core

in vec3 v_Normal;
in vec3 v_FragPos;
in vec2 v_TexCoord;
in float v_Height;

out vec4 FragColor;

uniform sampler2D u_DiffuseMap;        // Текстурный сэмплер
uniform float u_TextureScale = 1.0;    // Масштаб текстуры
uniform vec3 u_LightPos = vec3(100.0, 200.0, 100.0);
uniform vec3 u_LightColor = vec3(1.0, 1.0, 0.95);
uniform float u_MaxHeight = 80.0;

// Настройки материала
uniform vec3 u_Specular = vec3(0.3);
uniform float u_Shininess = 16.0;

void main() {
    // --- Текстура ---
    vec2 scaledTexCoord = v_TexCoord * u_TextureScale;
    vec4 textureColor = texture(u_DiffuseMap, scaledTexCoord);
    
    // Если текстура не загружена, используем цвет по высоте
    if (textureColor.a == 0.0) {
        float h = v_Height / u_MaxHeight;
        vec3 lowColor  = vec3(0.15, 0.15, 0.15);  // темно-серый
        vec3 highColor = vec3(0.45, 0.45, 0.45);  // серый
        textureColor = vec4(mix(lowColor, highColor, h), 1.0);
    }

    // --- Освещение ---
    vec3 norm = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    
    // Диффузная составляющая
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor;

    // Спеулярная составляющая
    vec3 viewDir = normalize(-v_FragPos); // Для простоты используем направление камеры
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    vec3 specular = u_Specular * spec * u_LightColor;

    // Ambient составляющая
    float ambient = 0.4;
    vec3 lighting = ambient + diffuse + specular;

    // --- Финал ---
    vec3 result = lighting * textureColor.rgb;
    FragColor = vec4(result, 1.0);
}