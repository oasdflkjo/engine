#include "camera.h"
#include <math.h>

#define ZOOM_DEFAULT 45.0f
#define ZOOM_MIN 1.0f
#define ZOOM_MAX 150.0f
#define ZOOM_SPEED 5.0f
#define ZOOM_FACTOR 0.1f
#define PAN_SPEED 0.005f
#define SMOOTH_FACTOR 5.0f

void camera_init(Camera* camera, int screen_width, int screen_height) {
    // Set initial position
    vec3 initial_pos = {0.0f, 0.0f, 5.0f};
    glm_vec3_copy(initial_pos, camera->position);
    glm_vec3_copy(initial_pos, camera->target_position);
    
    // Set initial orientation
    glm_vec3_copy((vec3){0.0f, 0.0f, -1.0f}, camera->front);
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, camera->up);
    
    // Calculate right vector
    glm_vec3_cross(camera->front, camera->up, camera->right);
    glm_vec3_normalize(camera->right);
    
    // Set camera parameters
    camera->aspect_ratio = (float)screen_width / (float)screen_height;
    camera->zoom = ZOOM_DEFAULT;
    camera->target_zoom = ZOOM_DEFAULT;
    camera->zoom_speed = ZOOM_SPEED;
    camera->pan_speed = PAN_SPEED;
    camera->is_transitioning = false;
}

void camera_get_view_matrix(Camera* camera, mat4 view) {
    vec3 center;
    glm_vec3_add(camera->position, camera->front, center);
    glm_lookat(camera->position, center, camera->up, view);
}

void camera_get_projection_matrix(Camera* camera, mat4 projection) {
    glm_perspective(glm_rad(camera->zoom), camera->aspect_ratio, 0.1f, 100.0f, projection);
}

void camera_process_pan(Camera* camera, float xoffset, float yoffset) {
    xoffset *= camera->pan_speed;
    yoffset *= camera->pan_speed;

    vec3 move = {0};
    vec3 scaled_right, scaled_up;

    glm_vec3_scale(camera->right, -xoffset, scaled_right);
    glm_vec3_scale(camera->up, yoffset, scaled_up);

    glm_vec3_add(scaled_right, move, move);
    glm_vec3_add(scaled_up, move, move);

    glm_vec3_add(camera->position, move, camera->position);
}

void camera_process_scroll(Camera* camera, float yoffset) {
    float zoom_factor = camera->target_zoom * ZOOM_FACTOR;
    camera->target_zoom -= yoffset * zoom_factor;
    
    if (camera->target_zoom < ZOOM_MIN)
        camera->target_zoom = ZOOM_MIN;
    if (camera->target_zoom > ZOOM_MAX)
        camera->target_zoom = ZOOM_MAX;
}

void camera_update(Camera* camera, float deltaTime) {
    // Always smoothly interpolate zoom
    float zoom_diff = camera->target_zoom - camera->zoom;
    camera->zoom += zoom_diff * SMOOTH_FACTOR * deltaTime;
    
    if (camera->is_transitioning) {
        // Smoothly interpolate position
        vec3 diff;
        glm_vec3_sub(camera->target_position, camera->position, diff);
        
        // If we're close enough to target, snap to it
        if (glm_vec3_norm(diff) < 0.001f) {
            glm_vec3_copy(camera->target_position, camera->position);
            camera->is_transitioning = false;
        } else {
            // Smoothly move towards target
            glm_vec3_scale(diff, SMOOTH_FACTOR * deltaTime, diff);
            glm_vec3_add(camera->position, diff, camera->position);
        }
    }
}

void camera_reset(Camera* camera) {
    // Set target position to default values
    vec3 default_pos = {0.0f, 0.0f, 5.0f};
    glm_vec3_copy(default_pos, camera->target_position);
    camera->target_zoom = ZOOM_DEFAULT;
    camera->is_transitioning = true;
}
