#ifndef HUD_H
#define HUD_H

#include <GLFW/glfw3.h>
#include "particle_system.h"

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

// Declare all functions with C linkage
void hud_init(HUD* hud, ParticleSystem* ps);
void hud_render(HUD* hud);
void hud_cleanup(HUD* hud);
void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime);
void hud_toggle(HUD* hud);

#ifdef __cplusplus
}
#endif

#endif // HUD_H
