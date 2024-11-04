#include "camera.h"
#include <math.h>

void camera_init(Camera* camera, int width, int height) {
    camera->position[0] = 0.0f;
    camera->position[1] = 0.0f;
    camera->position[2] = 50.0f;  // Initial zoom level
    camera->target[0] = 0.0f;
    camera->target[1] = 0.0f;
    camera->target[2] = 0.0f;
    camera->up[0] = 0.0f;
    camera->up[1] = 1.0f;
    camera->up[2] = 0.0f;
    camera->width = width;
    camera->height = height;
    camera->zoom_speed = 0.1f;     // Reduced for smoother zooming
    camera->pan_speed = 1.0f;
    camera->target_zoom = camera->position[2];
    camera->zoom_smoothness = 0.1f;
    camera->on_target_changed = NULL;
    camera->user_data = NULL;
}

void camera_process_pan(Camera* camera, float xoffset, float yoffset) {
    // Calculate screen-to-world ratio based on current zoom level
    float screen_to_world = camera->position[2] * 2.0f / camera->height;  // World units per screen pixel
    
    // Apply pan movement scaled by screen-to-world ratio
    float dx = -xoffset * screen_to_world * camera->pan_speed;
    float dy = yoffset * screen_to_world * camera->pan_speed;
    
    // Update both position and target
    camera->position[0] += dx;
    camera->position[1] += dy;
    
    // Update target position to be at the center of the view
    camera->target[0] = camera->position[0];
    camera->target[1] = camera->position[1];
    camera->target[2] = 0.0f;  // Keep target on the plane
    
    // Notify about target change
    if (camera->on_target_changed) {
        camera->on_target_changed(camera->target[0], camera->target[1], camera->user_data);
    }
}

void camera_process_scroll(Camera* camera, float yoffset) {
    // Use exponential scaling for more natural zoom feel
    float zoom_factor = powf(1.2f, -yoffset);
    camera->target_zoom = fmaxf(0.1f, 
                               fminf(camera->target_zoom * zoom_factor, 
                                    500.0f));
}

void camera_update(Camera* camera, float deltaTime) {
    // Smoothly interpolate current zoom to target zoom
    camera->position[2] += (camera->target_zoom - camera->position[2]) * camera->zoom_smoothness;
}

void camera_get_view_matrix(Camera* camera, mat4 view) {
    glm_lookat(camera->position, camera->target, camera->up, view);
}

void camera_get_projection_matrix(Camera* camera, mat4 projection) {
    float aspect = (float)camera->width / (float)camera->height;
    glm_ortho(-aspect * camera->position[2], 
              aspect * camera->position[2], 
              -camera->position[2], 
              camera->position[2], 
              -1000.0f, 1000.0f, 
              projection);
}

void camera_reset(Camera* camera) {
    camera->position[0] = 0.0f;
    camera->position[1] = 0.0f;
    camera->position[2] = 50.0f;
    
    // Reset target to center
    camera->target[0] = 0.0f;
    camera->target[1] = 0.0f;
    camera->target[2] = 0.0f;
    
    camera->target_zoom = camera->position[2];
    
    // Notify about target change
    if (camera->on_target_changed) {
        camera->on_target_changed(camera->target[0], camera->target[1], camera->user_data);
    }
}

void camera_set_target_callback(Camera* camera, CameraTargetChangedCallback callback, void* user_data) {
    camera->on_target_changed = callback;
    camera->user_data = user_data;
}
