#include "hud.h"
#include "imgui.h"
#include <stdio.h>

extern "C" {

void hud_init(HUD* hud) {
    hud->stats.fps = 0.0f;
    hud->stats.particleCount = 0;
    hud->stats.frameTime = 0.0f;
    hud->stats.deltaTime = 0.0f;
}

void hud_render(HUD* hud) {
    // Set window position to top-right corner with some padding
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 250, 30), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                  ImGuiWindowFlags_AlwaysAutoResize |
                                  ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoFocusOnAppearing |
                                  ImGuiWindowFlags_NoNav |
                                  ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("HUD", NULL, window_flags)) {
        ImGui::Text("FPS: %.1f", hud->stats.fps);
        ImGui::Text("Frame Time: %.2f ms", hud->stats.frameTime);
        ImGui::Text("Delta Time: %.3f ms", hud->stats.deltaTime * 1000.0f);
        ImGui::Text("Particle Count: %d", hud->stats.particleCount);
    }
    ImGui::End();
}

void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime) {
    hud->stats.fps = fps;
    hud->stats.particleCount = particleCount;
    hud->stats.frameTime = frameTime;
    hud->stats.deltaTime = deltaTime;
}

void hud_cleanup(HUD* hud) {
    // Nothing to cleanup for now
}

} // extern "C" 