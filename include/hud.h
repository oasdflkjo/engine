#ifndef HUD_H
#define HUD_H

#include "particle_system.h"
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ParticleSystem* particleSystem;
    GLFWwindow* window;
    float fps;
    int particleCount;
    float frameTime;
    float deltaTime;
} HUD;

void hud_init(HUD* hud, ParticleSystem* ps);
void hud_render(HUD* hud);
void hud_cleanup(HUD* hud);
void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime);

#ifdef __cplusplus
}
#endif

#endif // HUD_H 