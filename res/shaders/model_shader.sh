#shader vertex
#version 330 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texCoords;
layout (location = 3) in vec3 a_tangent;
layout (location = 4) in vec3 a_bitangent;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    FragPos = vec3(u_model * vec4(a_position, 1.0));
    Normal = mat3(transpose(inverse(u_model))) * a_normal;  
    TexCoords = a_texCoords;
    
    gl_Position = u_projection * u_view * vec4(FragPos, 1.0);
}

#shader fragment
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;\
struct Material {
    vec3 diffuse;
    vec3 specular;
    float shininess;
    sampler2D diffuse_map;
    sampler2D specular_map;
    sampler2D normal_map;
    bool use_diffuse_map;
    bool use_specular_map;
    bool use_normal_map;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

void main() {
    // Ambient
    vec3 ambient = light.ambient;
    if (material.use_diffuse_map) {
        ambient *= texture(material.diffuse_map, TexCoord).rgb;
    } else {
        ambient *= material.diffuse;
    }
    
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    vec3 diffuse = light.diffuse * diff;
    if (material.use_diffuse_map) {
        diffuse *= texture(material.diffuse_map, TexCoord).rgb;
    } else {
        diffuse *= material.diffuse;
    }
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    vec3 specular = light.specular * spec;
    if (material.use_specular_map) {
        specular *= texture(material.specular_map, TexCoord).rgb;
    } else {
        specular *= material.specular;
    }
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}