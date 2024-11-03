#ifndef UI_H
#define UI_H

#include <glad/glad.h>  // Must come before GLFW
#include <GLFW/glfw3.h>

typedef struct World World;

typedef struct {
    GLFWwindow* window;
    bool show_ui;
} UI;

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(UI* ui, GLFWwindow* window);
void ui_render(UI* ui, World* world);
void ui_end_frame(void);
void ui_cleanup(UI* ui);
void ui_toggle(UI* ui);

#ifdef __cplusplus
}
#endif

#endif // UI_H 