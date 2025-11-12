#pragma once
#include <glm/glm.hpp>
#include <string>
struct FireParams {
    // ТЕКСТУРЫ ДЛЯ КАЖДОГО ЭФФЕКТА
    std::string fireTexturePath = "res/textures/fire_texture3.png";
    std::string smokeTexturePath = "res/textures/smoke_texture.png";
    std::string sparksTexturePath = "res/textures/sparks_texture.png";
    std::string ashTexturePath = "res/textures/ash_texture.png";
    std::string heatTexturePath = "res/textures/heat_texture.png";
    
    struct EffectParams {
        bool enabled = true;
        float particleSize = 2.0f;
        float sizeVariation = 0.5f;
        float animationSpeed = 1.0f;
        float lifetime = 3.0f;
        float spawnRate = 100.0f;
        float riseSpeed = 2.0f;
        float turbulence = 0.5f;
        float windInfluence = 1.0f;
        float spread = 1.0f;

        // ЦВЕТА
        glm::vec3 baseColor = {1.0f, 0.3f, 0.1f};
        glm::vec3 endColor = {1.0f, 0.8f, 0.2f};

        // УНИВЕРСАЛЬНЫЕ ЦВЕТОВЫЕ ПАРАМЕТРЫ ДЛЯ ВСЕХ ЭФФЕКТОВ
        float redIntensity = 1.0f;
        float yellowReduction = 0.3f;
        float alphaMultiplier = 1.0f;
        float textureInfluence = 0.7f;
        float colorVariation = 0.0f;
    };
    
    EffectParams fire;
    EffectParams smoke;
    EffectParams sparks;
    EffectParams ash;
    EffectParams heat;
};