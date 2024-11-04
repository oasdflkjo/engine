#include "hud.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static bool imgui_initialized = false;
static bool show_hud = true;

void hud_init(HUD* hud, ParticleSystem* ps) {
    hud->particleSystem = ps;
    hud->fps = 0.0f;
    hud->particleCount = 0;
    hud->frameTime = 0.0f;
    hud->deltaTime = 0.0f;

    // Initialize ImGui only once
    if (!imgui_initialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // Increase font size and enable antialiasing
        io.FontGlobalScale = 1.5f;
        io.Fonts->AddFontDefault();
        io.FontAllowUserScaling = true;
        
        // Enable antialiasing on fonts
        io.Fonts->Build();
        io.Fonts->TexDesiredWidth = 512;
        io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;
        
        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(hud->window, true);
        ImGui_ImplOpenGL3_Init("#version 430");
        
        // Setup Dear ImGui style with antialiasing
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 1.0f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;
        style.AntiAliasedLinesUseTex = true;
        
        imgui_initialized = true;
    }
}

void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime) {
    hud->fps = fps;
    hud->particleCount = particleCount;
    hud->frameTime = frameTime;
    hud->deltaTime = deltaTime;
}

void hud_render(HUD* hud) {
    if (!show_hud) return;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Esc")) {
                glfwSetWindowShouldClose(hud->window, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Toggle HUD", "H")) {
                hud_toggle(hud);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Controls window
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    
    ImGui::Begin("Particle System Controls", nullptr, window_flags);
    
    // Display performance stats
    ImGui::Text("FPS: %.1f", hud->fps);
    ImGui::Text("Frame Time: %.2f ms", hud->frameTime);
    ImGui::Text("Particle Count: %d", hud->particleCount);
    
    ImGui::Separator();
    
    // Simulation Controls
    ImGui::Text("Simulation Controls");
    
    float timeScale = hud->particleSystem->timeScale;
    if (ImGui::SliderFloat("Time Scale", &timeScale, 0.1f, 2.0f)) {
        particle_system_set_time_scale(hud->particleSystem, timeScale);
    }
    
    float attractionStrength = hud->particleSystem->attractionStrength;
    if (ImGui::SliderFloat("Attraction Strength", &attractionStrength, 0.1f, 10.0f)) {
        particle_system_set_attraction_strength(hud->particleSystem, attractionStrength);
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void hud_cleanup(HUD* hud) {
    if (imgui_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized = false;
    }
}

void hud_toggle(HUD* hud) {
    show_hud = !show_hud;
}