#include "hud.h"
#include "imgui.h"

void hud_init(HUD* hud, ParticleSystem* ps) {
    hud->particleSystem = ps;
    hud->fps = 0.0f;
    hud->particleCount = 0;
    hud->frameTime = 0.0f;
    hud->deltaTime = 0.0f;
}

void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime) {
    hud->fps = fps;
    hud->particleCount = particleCount;
    hud->frameTime = frameTime;
    hud->deltaTime = deltaTime;
}

void hud_render(HUD* hud) {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    
    ImGui::Begin("Particle System Controls", nullptr, window_flags);
    
    // Display performance stats
    ImGui::Text("FPS: %.1f", hud->fps);
    ImGui::Text("Frame Time: %.2f ms", hud->frameTime);
    ImGui::Text("Particle Count: %d", hud->particleCount);
    
    ImGui::Separator();
    
    // Force parameters
    ImGui::Text("Force Parameters");
    float forceScale = hud->particleSystem->forceScale;
    if (ImGui::SliderFloat("Force Scale", &forceScale, 0.0f, 500.0f)) {
        particle_system_set_force_scale(hud->particleSystem, forceScale);
    }
    
    float maxForce = hud->particleSystem->maxForce;
    if (ImGui::SliderFloat("Max Force", &maxForce, 0.0f, 1000.0f)) {
        hud->particleSystem->maxForce = maxForce;
    }

    float minDistance = hud->particleSystem->minDistance;
    if (ImGui::SliderFloat("Min Distance", &minDistance, 0.0001f, 1.0f, "%.4f")) {
        hud->particleSystem->minDistance = minDistance;
    }

    // Velocity parameters
    ImGui::Separator();
    ImGui::Text("Velocity Parameters");
    
    float terminalVel = hud->particleSystem->terminalVelocity;
    if (ImGui::SliderFloat("Terminal Velocity", &terminalVel, 0.0f, 200.0f)) {
        particle_system_set_terminal_velocity(hud->particleSystem, terminalVel);
    }

    float damping = hud->particleSystem->damping;
    if (ImGui::SliderFloat("Damping", &damping, 0.0f, 1.0f)) {
        particle_system_set_damping(hud->particleSystem, damping);
    }

    // Mouse interaction parameters
    ImGui::Separator();
    ImGui::Text("Mouse Interaction");
    
    float mouseRadius = hud->particleSystem->mouseForceRadius;
    if (ImGui::SliderFloat("Mouse Force Radius", &mouseRadius, 0.0f, 20.0f)) {
        hud->particleSystem->mouseForceRadius = mouseRadius;
    }

    float mouseStrength = hud->particleSystem->mouseForceStrength;
    if (ImGui::SliderFloat("Mouse Force Strength", &mouseStrength, 0.0f, 5.0f)) {
        hud->particleSystem->mouseForceStrength = mouseStrength;
    }

    // Add presets
    ImGui::Separator();
    ImGui::Text("Presets");
    if (ImGui::Button("Gentle Attraction")) {
        hud->particleSystem->forceScale = 100.0f;
        hud->particleSystem->maxForce = 150.0f;
        hud->particleSystem->damping = 0.8f;
        hud->particleSystem->terminalVelocity = 50.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Chaos")) {
        hud->particleSystem->forceScale = 400.0f;
        hud->particleSystem->maxForce = 800.0f;
        hud->particleSystem->damping = 0.2f;
        hud->particleSystem->terminalVelocity = 200.0f;
    }

    ImGui::End();
}

void hud_cleanup(HUD* hud) {
    // Nothing to clean up
}