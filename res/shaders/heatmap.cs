#version 430

layout(local_size_x = 16, local_size_y = 16) in;

struct Float3 {
    float x, y, z;
};

struct FireCollider {
    Float3 heat_points[8];
    float temperatures[8];
    float moisture[8];
    float ignition_temp[8];
    float intensities[8];
    float fuels[8];
    int point_count;
};

struct Agent {
    FireCollider fire_collider;
    int type;
    float padding[3];
};

layout(std430, binding = 0) buffer AgentsSSBO {
    Agent agents[];
};

layout(std430, binding = 1) buffer PixelsSSBO {
    uint pixels[];
};

uniform float u_AmbientTemp;
uniform float u_MinTemp;
uniform float u_MaxTemp;
uniform float u_WorldMinX;
uniform float u_WorldMaxX;
uniform float u_WorldMinZ;
uniform float u_WorldMaxZ;
uniform int u_AgentCount;
uniform int u_TextureWidth;
uniform int u_TextureHeight;

vec3 hslToRgb(float h, float s, float l) {
    h = mod(h, 360.0);
    s = clamp(s, 0.0, 1.0);
    l = clamp(l, 0.0, 1.0);
    
    float c = (1.0 - abs(2.0 * l - 1.0)) * s;
    float x = c * (1.0 - abs(mod(h / 60.0, 2.0) - 1.0));
    float m = l - c / 2.0;
    
    vec3 rgb;
    if (h < 60.0) {
        rgb = vec3(c, x, 0.0);
    } else if (h < 120.0) {
        rgb = vec3(x, c, 0.0);
    } else if (h < 180.0) {
        rgb = vec3(0.0, c, x);
    } else if (h < 240.0) {
        rgb = vec3(0.0, x, c);
    } else if (h < 300.0) {
        rgb = vec3(x, 0.0, c);
    } else {
        rgb = vec3(c, 0.0, x);
    }
    
    return rgb + vec3(m);
}

vec3 colorFrom01(float t) {
    t = clamp(t, 0.0, 1.0);
    float hue = 240.0 * (1.0 - t);
    return hslToRgb(hue, 1.0, 0.5);
}

int worldToGrid(float x, float worldMin, float worldMax, int size) {
    float t = (x - worldMin) / (worldMax - worldMin);
    return int(t * float(size));
}

uint packColor(vec3 color, float alpha) {
    uint r = uint(color.r * 255.0);
    uint g = uint(color.g * 255.0);
    uint b = uint(color.b * 255.0);
    uint a = uint(alpha * 255.0);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    
    if (pixelCoord.x >= u_TextureWidth || pixelCoord.y >= u_TextureHeight) {
        return;
    }
    
    int pixelIndex = pixelCoord.y * u_TextureWidth + pixelCoord.x;
    float cellTemp = u_AmbientTemp;
    
    // Обрабатываем всех агентов
    for (int i = 0; i < u_AgentCount; i++) {
        Agent agent = agents[i];
        if (agent.fire_collider.point_count == 0) continue;
        
        for (int j = 0; j < agent.fire_collider.point_count; j++) {
            Float3 point = agent.fire_collider.heat_points[j];
            float temp = agent.fire_collider.temperatures[j];
            float moisture = agent.fire_collider.moisture[j];
            
            float coolingEffect = moisture * moisture * 50.0;
            float adjustedTemp = temp - coolingEffect;
            
            int gx = worldToGrid(point.x, u_WorldMinX, u_WorldMaxX, u_TextureWidth);
            int gz = worldToGrid(point.z, u_WorldMinZ, u_WorldMaxZ, u_TextureHeight);
            
            if (gx == pixelCoord.x && gz == pixelCoord.y) {
                cellTemp += (adjustedTemp - u_AmbientTemp) * 0.1;
            }
        }
    }
    
    // Нормализуем и упаковываем цвет
    float t = (cellTemp - u_MinTemp) / (u_MaxTemp - u_MinTemp);
    t = clamp(t, 0.0, 1.0);
    vec3 color = colorFrom01(t);
    uint packedColor = packColor(color, 0.6);
    
    pixels[pixelIndex] = packedColor;
}