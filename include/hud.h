#ifndef HUD_H
#define HUD_H

#include <GLFW/glfw3.h>

typedef struct {
    float fps;
    int particleCount;
    float frameTime;
    float deltaTime;
} HUDStats;

typedef struct {
    HUDStats stats;
} HUD;

#ifdef __cplusplus
extern "C" {
#endif

void hud_init(HUD* hud);
void hud_render(HUD* hud);
void hud_update_stats(HUD* hud, float fps, int particleCount, float frameTime, float deltaTime);
void hud_cleanup(HUD* hud);

#ifdef __cplusplus
}
#endif

#endif // HUD_H 