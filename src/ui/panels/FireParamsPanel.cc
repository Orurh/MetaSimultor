#include "FireParamsPanel.h"
#include "graphics/Renderer.h"
#include "ui/EditorContext.h"
#include <imgui.h>
#include <iostream>

namespace ui {

void FireParamsPanel::Draw(EditorContext &ctx) {
  if (!ImGui::Begin(Id())) {
    ImGui::End();
    return;
  }

  if (ImGui::Button("Reset to Default")) {
    m_params = FireParams();
  }
  ImGui::SameLine();
  if (ImGui::Button("Apply to Shader")) {
    // Применяем параметры к шейдеру
    auto &renderer = engine::Renderer::Instance();
    renderer.SetFireParams(m_params);
  }

  ImGui::SeparatorText("Main Parameters");

  ImGui::SliderFloat("Base Intensity", &m_params.baseIntensity, 0.1f, 5.0f);
  ImGui::SliderFloat("Particle Size", &m_params.particleSize, 0.5f, 10.0f);
  ImGui::SliderFloat("Size Variation", &m_params.sizeVariation, 0.0f, 2.0f);

  ImGui::SeparatorText("Animation");
  ImGui::SliderFloat("Animation Speed", &m_params.animationSpeed, 0.1f, 5.0f);
  ImGui::SliderFloat("Particle Lifetime", &m_params.particleLifetime, 0.5f,
                     10.0f);
  ImGui::SliderFloat("Spawn Rate", &m_params.spawnRate, 10.0f, 1000.0f);

  ImGui::SeparatorText("Movement");
  ImGui::SliderFloat("Rise Speed", &m_params.riseSpeed, 0.1f, 10.0f);
  ImGui::SliderFloat("Turbulence", &m_params.turbulence, 0.0f, 2.0f);
  ImGui::SliderFloat("Wind Influence", &m_params.windInfluence, 0.0f, 5.0f);

  ImGui::SeparatorText("Visual Effects");
  ImGui::SliderFloat("Red Intensity", &m_params.redIntensity, 0.0f, 2.0f);
  ImGui::SliderFloat("Yellow Reduction", &m_params.yellowReduction, 0.0f, 1.0f);
  ImGui::SliderFloat("Alpha Multiplier", &m_params.alphaMultiplier, 0.1f, 3.0f);

  ImGui::SeparatorText("Particle Distribution");
  ImGui::SliderFloat("Horizontal Spread", &m_params.horizontalSpread, 0.1f,
                     2.0f);
  ImGui::SliderFloat("Vertical Spread", &m_params.verticalSpread, 0.1f, 2.0f);

  ImGui::SeparatorText("Colors");
  ImGui::ColorEdit3("Base Color", &m_params.baseColor[0]);
  ImGui::ColorEdit3("End Color", &m_params.endColor[0]);
  ImGui::SeparatorText("Fire Texture");

  static char texturePath[256] = "res/textures/fire_texture3.png";
  if (ImGui::InputText("Texture Path", texturePath, sizeof(texturePath))) {
    m_params.fireTexturePath = texturePath;
  }

  if (ImGui::Button("Load Texture")) {
    m_params.fireTexturePath = texturePath;
    std::cout << "here" << std::endl;
  }

  ImGui::SliderFloat("Texture Influence", &m_params.textureInfluence, 0.0f,
                     1.0f, "%.2f");
  ImGui::TextDisabled("0.0 = only params color, 1.0 = only texture color");
  ImGui::End();
}

} // namespace ui