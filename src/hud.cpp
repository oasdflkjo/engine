#include "hud.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "particle_system.h"
#include <stdio.h>

static bool show_window = true;
static bool show_menu_bar = true;
static Simulation* current_simulation = nullptr;

void hud_init(HUD* hud, Simulation* simulation) {
    current_simulation = simulation;  // Store simulation pointer
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(hud->window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    // Get window size for proper scaling
    int width, height;
    glfwGetWindowSize(hud->window, &width, &height);
    float scale = (float)height / 1080.0f; // Base scale on 1080p

    // Setup font with proper scaling
    io.Fonts->Clear();
    ImFontConfig font_config;
    font_config.SizePixels = 13.0f * scale;
    font_config.OversampleH = 8;
    font_config.OversampleV = 8;
    font_config.PixelSnapH = true;
    io.Fonts->AddFontDefault(&font_config);
    io.Fonts->Build();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Scale style
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);
    
    // Customize colors for better visibility
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
}

void hud_cleanup(HUD* hud) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime) {
    hud->fps = fps;
    hud->particleCount = particleCount;
    hud->frameTime = frameTime;
    hud->deltaTime = deltaTime;
}

void hud_toggle(HUD* hud) {
    show_window = !show_window;
    show_menu_bar = !show_menu_bar;
}

void hud_render(HUD* hud) {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main menu bar (only if visible)
    if (show_menu_bar) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(hud->window, true);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    // Stats and controls window
    if (show_window) {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 window_pos(10.0f, show_menu_bar ? 30.0f : 10.0f);
        ImVec2 window_size(250.0f * (io.DisplaySize.y / 1080.0f), 0.0f);
        
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
        
        ImGui::Begin("Simulation Controls", &show_window, 
                    ImGuiWindowFlags_NoSavedSettings | 
                    ImGuiWindowFlags_AlwaysAutoResize);
        
        // Performance stats
        if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", hud->fps);
            ImGui::Text("Frame Time: %.3f ms", hud->frameTime);
            ImGui::Text("Delta Time: %.3f ms", hud->deltaTime * 1000.0f);
            ImGui::Text("Particle Count: %d", hud->particleCount);
        }
        
        // Simulation controls
        if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Time scale control with extended range
            float timeScale = simulation_get_time_scale(current_simulation);
            if (ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 20.0f, "%.2f")) {
                simulation_set_time_scale(current_simulation, timeScale);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Adjust simulation speed (0x - 20x)");
            }

            // Speed preset buttons
            if (ImGui::Button("0.1x")) {
                simulation_set_time_scale(current_simulation, 0.1f);
            }
            ImGui::SameLine();
            if (ImGui::Button("1x")) {
                simulation_set_time_scale(current_simulation, 1.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("5x")) {
                simulation_set_time_scale(current_simulation, 5.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("20x")) {
                simulation_set_time_scale(current_simulation, 20.0f);
            }

            // Attraction strength control
            float attractionStrength = simulation_get_attraction_strength(current_simulation);
            if (ImGui::SliderFloat("Attraction", &attractionStrength, 0.0f, 2.0f, "%.2f")) {
                simulation_set_attraction_strength(current_simulation, attractionStrength);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Adjust attraction force strength");
            }
        }
        
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}