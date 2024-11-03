#ifndef UI_H
#define UI_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GLFWwindow* window;
    bool show_ui;
} UI;

void ui_init(UI* ui, GLFWwindow* window);
void ui_render(UI* ui, void* world);
void ui_cleanup(UI* ui);
void ui_toggle(UI* ui);
void ui_end_frame(void);
bool ui_is_visible(UI* ui);

#ifdef __cplusplus
}
#endif

#endif // UI_H 