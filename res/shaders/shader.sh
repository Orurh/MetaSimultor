#shader vertex
#version 330 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_instance_model_0;
layout (location = 2) in vec4 a_instance_model_1;
layout (location = 3) in vec4 a_instance_model_2;
layout (location = 4) in vec4 a_instance_model_3;
layout (location = 5) in vec3 a_instance_color;

uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_color;

void main() {
    mat4 model = mat4(
        a_instance_model_0,
        a_instance_model_1, 
        a_instance_model_2,
        a_instance_model_3
    );
    
    gl_Position = u_projection * u_view * model * vec4(a_position, 1.0);
    v_color = a_instance_color;
}

#shader fragment
#version 330

out vec4 FragColor;
in vec3 v_color;
void main() {
    FragColor = vec4(v_color, 1.0f);
}