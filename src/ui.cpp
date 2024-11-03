#include "ui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "world.h"

static bool imgui_initialized = false;

void ui_init(UI* ui, GLFWwindow* window) {
    ui->window = window;
    ui->show_ui = true;
    
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
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 430");
        
        // Setup Dear ImGui style with antialiasing
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 1.0f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;
        style.AntiAliasedLinesUseTex = true;
        
        // Customize colors
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        
        imgui_initialized = true;
    }
}

void ui_render(UI* ui, void* world_ptr) {
    World* world = (World*)world_ptr;
    
    // Always start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Only show menu bar if UI is visible
    if (ui->show_ui) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Toggle Controls", "H")) {
                    ui_toggle(ui);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
}

void ui_cleanup(UI* ui) {
    if (imgui_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized = false;
    }
}

void ui_toggle(UI* ui) {
    ui->show_ui = !ui->show_ui;
}

void ui_end_frame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ui_is_visible(UI* ui) {
    return ui->show_ui;
}