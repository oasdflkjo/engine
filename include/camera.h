#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>

typedef struct {
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    float aspect_ratio;
    float zoom;
    float target_zoom;
    float zoom_speed;
    float pan_speed;
} Camera;

// Initialize camera with screen dimensions
void camera_init(Camera* camera, int screen_width, int screen_height);

// Get view matrix for the camera
void camera_get_view_matrix(Camera* camera, mat4 view);

// Get projection matrix for the camera
void camera_get_projection_matrix(Camera* camera, mat4 projection);

// Process mouse pan (middle mouse button drag)
void camera_process_pan(Camera* camera, float xoffset, float yoffset);

// Process mouse scroll
void camera_process_scroll(Camera* camera, float yoffset);

// Update camera (for smooth zoom)
void camera_update(Camera* camera, float deltaTime);

// Reset camera to initial position
void camera_reset(Camera* camera);

#endif
