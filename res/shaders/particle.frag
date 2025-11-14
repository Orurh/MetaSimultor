#version 330 core

in vec2 v_texcoord;
in vec4 v_color;

out vec4 FragColor;

void main() {
    // Простая круговая форма
    vec2 centered = v_texcoord - 0.5;
    float dist = length(centered);
    
    if (dist > 0.5) {
        discard;
    }
    
    // Плавные края
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    FragColor = v_color * alpha;
    
    if (FragColor.a < 0.01) {
        discard;
    }
}