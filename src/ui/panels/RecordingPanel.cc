
#include "RecordingPanel.h"
#include "core/Application.h"
#include <imgui.h>
namespace ui {
void RecordingPanel::Draw(EditorContext &ctx) {
  if (!ImGui::Begin("Scene Recorder")) {
    ImGui::End();
    return;
  }

  auto *app = ctx.m_app;
  if (!app)
    return;

  auto *recorder = app->GetSceneRecorder();
  if (!recorder) {
    ImGui::TextDisabled("Recorder not available");
    ImGui::End();
    return;
  }

  DrawRecorderUI(*recorder);
  ImGui::End();
}

void RecordingPanel::DrawRecorderUI(engine::SceneRecorder &recorder) {
  // Управление записью
  if (!recorder.IsRecording()) {
    if (ImGui::Button("Start Recording")) {
      recorder.StartRecording();
    }
  } else {
    if (ImGui::Button("Stop Recording")) {
      recorder.StopRecording();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "● RECORDING");
  }

  ImGui::Separator();

  // Воспроизведение
  // Воспроизведение
  if (recorder.GetSnapshotCount() > 0) {
    static float seekTime = 0.0f;
    static std::string lastError;

    float startTime = static_cast<float>(recorder.GetRecordingStartTime());
    float endTime = static_cast<float>(recorder.GetRecordingEndTime());

    // Устанавливаем seekTime в разумные пределы при первом открытии
    if (seekTime < startTime || seekTime > endTime) {
      seekTime = startTime;
    }

    if (ImGui::SliderFloat("Seek Time", &seekTime, startTime, endTime, "%.1f s")) {
      // НЕМЕДЛЕННОЕ применение при перетаскивании
      auto result = recorder.GetSceneState(seekTime);
      if (result) {
        recorder.ApplySceneState(*result);
        lastError.clear();
      } else {
        lastError = result.error();
      }
    }

    // Показываем ошибку если есть
    if (!lastError.empty()) {
      ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Error: %s", lastError.c_str());
    }

    // Кнопки для быстрой навигации
    ImGui::SameLine();
    if (ImGui::Button("<<")) {
      seekTime = startTime;
      auto result = recorder.GetSceneState(seekTime);
      if (result)
        recorder.ApplySceneState(*result);
    }

    ImGui::SameLine();
    if (ImGui::Button(">>")) {
      seekTime = endTime;
      auto result = recorder.GetSceneState(seekTime);
      if (result)
        recorder.ApplySceneState(*result);
    }
  }
}

}  // namespace ui
