#ifndef RECORDER_H
#define RECORDER_H

#include <stdbool.h>

typedef struct Recorder {
    bool enabled;
    int width;
    int height;
    int fps;
    void* impl;
} Recorder;

int recorder_target_fps_from_env(int fallbackFps);
bool recorder_init_from_env(Recorder* recorder, int width, int height, int fps);
void recorder_capture_frame(Recorder* recorder);
void recorder_cleanup(Recorder* recorder);

#endif // RECORDER_H
