#include "ParticleSystemPanel.h"
#include "core/scene/Scene.h"
#include <imgui.h>

namespace ui {

ParticleSystemPanel::ParticleSystemPanel() {
  m_currentEffect = engine::ParticleEffect::CreateDefault();
}

void ParticleSystemPanel::Draw(EditorContext &ctx) {
  ImGui::Begin("Particle Systems");

  // Кнопка добавления системы частиц
  if (ImGui::Button("Add Particle System")) {
    ImGui::OpenPopup("AddParticleSystemPopup");
  }

  if (ImGui::BeginPopup("AddParticleSystemPopup")) {
    DrawAddParticleSystemMenu(ctx);
    ImGui::EndPopup();
  }

  ImGui::SameLine();
  ImGui::Text("(%zu systems)",
              ctx.m_scene ? ctx.m_scene->GetParticleSystems().size() : 0);

  ImGui::Separator();

  // Список систем
  DrawParticleSystemList(ctx);

  // Управление выбранной системой и редактор эффектов
  if (m_selectedSystem) {
    DrawParticleSystemControls(ctx);
    ImGui::Separator();
    DrawEffectEditor();
  }

  ImGui::End();
}

void ParticleSystemPanel::DrawEffectEditor() {
  if (!m_selectedSystem || !m_selectedSystem->GetEffect())
    return;

  auto effect = m_selectedSystem->GetEffect();

  if (ImGui::CollapsingHeader("Main Module", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat("Duration", &effect->main.duration, 0.1f, 0.1f, 100.0f);
    ImGui::Checkbox("Looping", &effect->main.looping);
    ImGui::DragFloat("Start Delay", &effect->main.startDelay, 0.1f, 0.0f,
                     10.0f);

    ImGui::Separator();
    ImGui::Text("Start Values:");
    ImGui::DragFloat("Start Lifetime", &effect->main.startLifetime, 0.1f, 0.1f,
                     100.0f);
    ImGui::DragFloat3("Start Speed", &effect->main.startSpeed[0], 0.1f, -50.0f,
                      50.0f);
    ImGui::DragFloat3("Start Size", &effect->main.startSize[0], 0.1f, 0.01f,
                      10.0f);
    ImGui::ColorEdit4("Start Color", &effect->main.startColor[0]);
    ImGui::DragFloat("Gravity Modifier", &effect->main.gravityModifier, 0.1f,
                     -10.0f, 10.0f);

    ImGui::DragInt("Max Particles", (int *)&effect->main.maxParticles, 100, 100,
                   100000);
  }

  if (ImGui::CollapsingHeader("Emission Module")) {
    ImGui::Checkbox("Enabled", &effect->emission.enabled);
    ImGui::DragFloat("Rate Over Time", &effect->emission.rateOverTime, 1.0f,
                     0.0f, 1000.0f);
    ImGui::DragFloat("Rate Over Distance", &effect->emission.rateOverDistance,
                     0.1f, 0.0f, 10.0f);

    // Bursts
    if (ImGui::TreeNode("Bursts")) {
      for (size_t i = 0; i < effect->emission.bursts.size(); ++i) {
        auto &burst = effect->emission.bursts[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::DragFloat("Time", &burst.time, 0.1f, 0.0f,
                         effect->main.duration);
        ImGui::DragInt("Count", (int *)&burst.count, 1, 1, 1000);
        ImGui::DragInt("Cycles", (int *)&burst.cycles, 1, 1, 100);
        ImGui::DragFloat("Interval", &burst.interval, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Probability", &burst.probability, 0.1f, 0.0f, 1.0f);

        if (ImGui::Button("Remove")) {
          effect->emission.bursts.erase(effect->emission.bursts.begin() + i);
          ImGui::PopID();
          break;
        }
        ImGui::PopID();
      }

      if (ImGui::Button("Add Burst")) {
        effect->emission.bursts.push_back({});
      }
      ImGui::TreePop();
    }
  }

  if (ImGui::CollapsingHeader("Shape Module")) {
    ImGui::Checkbox("Enabled", &effect->shape.enabled);

    const char *shapeTypes[] = {"Sphere", "Cone", "Box", "Mesh"};
    ImGui::Combo("Shape Type", &effect->shape.shapeType, shapeTypes,
                 IM_ARRAYSIZE(shapeTypes));

    if (effect->shape.shapeType == 0) { // Sphere
      ImGui::DragFloat("Radius", &effect->shape.radius, 0.1f, 0.1f, 100.0f);
    } else if (effect->shape.shapeType == 1) { // Cone
      ImGui::DragFloat("Angle", &effect->shape.angle, 1.0f, 0.0f, 90.0f);
      ImGui::DragFloat("Radius", &effect->shape.radius, 0.1f, 0.1f, 100.0f);
    } else if (effect->shape.shapeType == 2) { // Box
      ImGui::DragFloat3("Box Size", &effect->shape.boxSize[0], 0.1f, 0.1f,
                        100.0f);
    }

    ImGui::DragFloat("Arc", &effect->shape.arc, 1.0f, 0.0f, 360.0f);
  }

  if (ImGui::CollapsingHeader("Velocity Over Lifetime")) {
    ImGui::Checkbox("Enabled##Velocity", &effect->velocityOverLifetime.enabled);
    ImGui::DragFloat3("Linear Velocity",
                      &effect->velocityOverLifetime.linear[0], 0.1f, -10.0f,
                      10.0f);
    ImGui::DragFloat3("Orbital Velocity",
                      &effect->velocityOverLifetime.orbital[0], 0.1f, -10.0f,
                      10.0f);
  }

  if (ImGui::CollapsingHeader("Color Over Lifetime")) {
    ImGui::Checkbox("Enabled##Color", &effect->colorOverLifetime.enabled);

    // Gradient editor (упрощенный)
    if (ImGui::TreeNode("Gradient")) {
      for (size_t i = 0; i < effect->colorOverLifetime.gradient.size(); ++i) {
        auto &key = effect->colorOverLifetime.gradient[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::DragFloat("Time", &key.time, 0.01f, 0.0f, 1.0f);
        ImGui::ColorEdit4("Color", &key.color[0]);

        if (ImGui::Button("Remove")) {
          effect->colorOverLifetime.gradient.erase(
              effect->colorOverLifetime.gradient.begin() + i);
          ImGui::PopID();
          break;
        }
        ImGui::PopID();
      }

      if (ImGui::Button("Add Key")) {
        effect->colorOverLifetime.gradient.push_back({0.0f, glm::vec4(1.0f)});
      }
      ImGui::TreePop();
    }
  }

  if (ImGui::CollapsingHeader("Size Over Lifetime")) {
    ImGui::Checkbox("Enabled##Size", &effect->sizeOverLifetime.enabled);

    if (ImGui::TreeNode("Size Curve")) {
      for (size_t i = 0; i < effect->sizeOverLifetime.curve.size(); ++i) {
        auto &key = effect->sizeOverLifetime.curve[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::DragFloat("Time", &key.time, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat3("Size", &key.size[0], 0.1f, 0.0f, 10.0f);

        if (ImGui::Button("Remove")) {
          effect->sizeOverLifetime.curve.erase(
              effect->sizeOverLifetime.curve.begin() + i);
          ImGui::PopID();
          break;
        }
        ImGui::PopID();
      }

      if (ImGui::Button("Add Key")) {
        effect->sizeOverLifetime.curve.push_back({0.0f, glm::vec3(1.0f)});
      }
      ImGui::TreePop();
    }
  }

  // Кнопки для применения изменений
  ImGui::Separator();
  if (ImGui::Button("Apply Changes")) {
    m_selectedSystem->SetEffect(effect);
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    effect = m_selectedSystem->GetEffect();
  }
}

void ParticleSystemPanel::DrawParticleSystemList(EditorContext &ctx) {
  auto *scene = ctx.m_scene;
  if (!scene) {
    ImGui::Text("No scene loaded");
    return;
  }

  const auto &systems = scene->GetParticleSystems();

  if (systems.empty()) {
    ImGui::Text("No particle systems");
    return;
  }

  ImGui::Text("Particle Systems:");
  ImGui::Separator();

  for (size_t i = 0; i < systems.size(); ++i) {
    auto &system = systems[i];
    bool isSelected = (system == m_selectedSystem);

    std::string label = "ParticleSystem " + std::to_string(i);
    if (system->IsPlaying()) {
      label += " ▶";
    } else {
      label += " ■";
    }

    if (ImGui::Selectable(label.c_str(), isSelected)) {
      m_selectedSystem = system;
    }

    // Кнопка удаления
    ImGui::SameLine();
    ImGui::PushID(static_cast<int>(i));
    if (ImGui::SmallButton("X")) {
      if (m_selectedSystem == system) {
        m_selectedSystem.reset();
      }
      scene->RemoveParticleSystem(system);
      ImGui::PopID();
      break;
    }
    ImGui::PopID();
  }
}

void ParticleSystemPanel::DrawParticleSystemControls(EditorContext &ctx) {
  ImGui::Separator();
  ImGui::Text("Selected Particle System");
  ImGui::Separator();

  // Кнопки управления
  if (ImGui::Button("Play")) {
    m_selectedSystem->Play();
  }
  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    m_selectedSystem->Stop();
  }
  ImGui::SameLine();
  if (ImGui::Button("Restart")) {
    m_selectedSystem->Stop();
    m_selectedSystem->Play();
  }

  // Статус
  ImGui::Text("Status: %s",
              m_selectedSystem->IsPlaying() ? "Playing" : "Stopped");
  ImGui::Text("Particles: %u", m_selectedSystem->GetParticleCount());

  // Быстрые пресеты
  ImGui::Separator();
  ImGui::Text("Quick Presets:");

  if (ImGui::Button("Fire Preset")) {
    auto effect = engine::ParticleEffect::CreateFire();
    m_selectedSystem->SetEffect(effect);
  }
  ImGui::SameLine();
  if (ImGui::Button("Smoke Preset")) {
    auto effect = engine::ParticleEffect::CreateSmoke();
    m_selectedSystem->SetEffect(effect);
  }
  ImGui::SameLine();
  if (ImGui::Button("Magic Preset")) {
    auto effect = engine::ParticleEffect::CreateMagic();
    m_selectedSystem->SetEffect(effect);
  }
}

void ParticleSystemPanel::DrawAddParticleSystemMenu(EditorContext &ctx) {
  ImGui::Text("Add Particle System:");
  ImGui::Separator();

  if (ImGui::MenuItem("Fire")) {
    auto effect = engine::ParticleEffect::CreateFire();
    auto system = std::make_shared<engine::ParticleSystem>();
    system->SetEffect(effect);
    system->Play();

    if (ctx.m_scene) {
      ctx.m_scene->AddParticleSystem(system);
      m_selectedSystem = system; // Автоматически выбираем новую систему
    }
  }

  if (ImGui::MenuItem("Smoke")) {
    auto effect = engine::ParticleEffect::CreateSmoke();
    auto system = std::make_shared<engine::ParticleSystem>();
    system->SetEffect(effect);
    system->Play();

    if (ctx.m_scene) {
      ctx.m_scene->AddParticleSystem(system);
      m_selectedSystem = system;
    }
  }

  if (ImGui::MenuItem("Magic")) {
    auto effect = engine::ParticleEffect::CreateMagic();
    auto system = std::make_shared<engine::ParticleSystem>();
    system->SetEffect(effect);
    system->Play();

    if (ctx.m_scene) {
      ctx.m_scene->AddParticleSystem(system);
      m_selectedSystem = system;
    }
  }

  if (ImGui::MenuItem("Custom (Empty)")) {
    auto effect = engine::ParticleEffect::CreateDefault();
    auto system = std::make_shared<engine::ParticleSystem>();
    system->SetEffect(effect);

    if (ctx.m_scene) {
      ctx.m_scene->AddParticleSystem(system);
      m_selectedSystem = system;
    }
  }

  // ... остальные методы как были ...

} // namespace ui
} // namespace ui
