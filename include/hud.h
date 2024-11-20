#ifndef HUD_H
#define HUD_H

typedef struct {
    bool show;
    float frameTime;
    float fps;
    int particleCount;
} Hud;

void hud_init(Hud* hud);
void hud_toggle(Hud* hud);
void hud_update_stats(Hud* hud, float deltaTime, int particleCount);
void hud_render(Hud* hud);
void hud_cleanup(Hud* hud);

#endif // HUD_H
