#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>

// Define callback type for target position changes
typedef void (*CameraTargetChangedCallback)(float x, float y, void* user_data);

typedef struct {
    vec3 position;
    vec3 target;
    vec3 up;
    int width;
    int height;
    float target_zoom;
    float zoom_speed;
    float pan_speed;
    float zoom_smoothness;
    
    // Callback for target position changes
    CameraTargetChangedCallback on_target_changed;
    void* user_data;
} Camera;

void camera_init(Camera* camera, int width, int height);
void camera_process_pan(Camera* camera, float xoffset, float yoffset);
void camera_process_scroll(Camera* camera, float yoffset);
void camera_update(Camera* camera, float deltaTime);
void camera_get_view_matrix(Camera* camera, mat4 view);
void camera_get_projection_matrix(Camera* camera, mat4 projection);
void camera_reset(Camera* camera);
void camera_set_target_callback(Camera* camera, CameraTargetChangedCallback callback, void* user_data);

#endif // CAMERA_H
