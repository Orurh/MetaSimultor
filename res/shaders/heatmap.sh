#shader vertex
#version 330 core
layout (location = 0) in vec2 a_Position;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_WorldMinX;
uniform float u_WorldMinZ;
uniform float u_WorldMaxX;
uniform float u_WorldMaxZ;
uniform float u_Height;  // Добавь эту uniform

out vec2 v_TexCoord;

void main()
{
    // Преобразуем нормализованные координаты в мировые
    float worldX = mix(u_WorldMinX, u_WorldMaxX, a_Position.x);
    float worldZ = mix(u_WorldMinZ, u_WorldMaxZ, a_Position.y);
    
    // Используем переданную высоту вместо 0.0
    vec3 worldPos = vec3(worldX, u_Height, worldZ);
    
    v_TexCoord = a_Position;
    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);
}


#shader fragment
#version 330 core
in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_HeatmapTexture;
uniform float u_Alpha;

void main()
{
    vec4 color = texture(u_HeatmapTexture, v_TexCoord);
    FragColor = vec4(color.rgb, color.a * u_Alpha);
}