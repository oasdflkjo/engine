#ifndef HUD_H
#define HUD_H

#include <GLFW/glfw3.h>
#include "simulation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HUD {
    GLFWwindow* window;
    float fps;
    int particleCount;
    float frameTime;
    float deltaTime;
} HUD;

void hud_init(HUD* hud, Simulation* simulation);
void hud_render(HUD* hud);
void hud_cleanup(HUD* hud);
void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime);
void hud_toggle(HUD* hud);

#ifdef __cplusplus
}
#endif

#endif // HUD_H 