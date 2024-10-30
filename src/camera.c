#include "camera.h"
#include <math.h>

#define ZOOM_DEFAULT 45.0f
#define ZOOM_MIN 1.0f
#define ZOOM_MAX 150.0f
#define ZOOM_SPEED 5.0f
#define ZOOM_FACTOR 0.1f
#define PAN_SPEED 0.005f

void camera_init(Camera* camera, int screen_width, int screen_height) {
    // Set initial position and orientation
    glm_vec3_copy((vec3){0.0f, 0.0f, 5.0f}, camera->position);
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
    float zoom_delta = (camera->target_zoom - camera->zoom) * camera->zoom_speed * deltaTime;
    camera->zoom += zoom_delta;
}

void camera_reset(Camera* camera) {
    // Reset to initial position and orientation
    glm_vec3_copy((vec3){0.0f, 0.0f, 5.0f}, camera->position);
    glm_vec3_copy((vec3){0.0f, 0.0f, -1.0f}, camera->front);
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, camera->up);
    
    // Recalculate right vector
    glm_vec3_cross(camera->front, camera->up, camera->right);
    glm_vec3_normalize(camera->right);
    
    // Reset zoom
    camera->zoom = ZOOM_DEFAULT;
    camera->target_zoom = ZOOM_DEFAULT;
}
