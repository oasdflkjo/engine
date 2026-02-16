#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "camera.h"
#include "world.h"
#include "hud.h"
#include "particle_system.h"
#include "recorder.h"

Camera camera;
World world;
float lastX = 0.0f;
float lastY = 0.0f;
bool middleMousePressed = false;
int windowWidth = 0;
int windowHeight = 0;
bool spaceHeld = false;
float savedAttractionStrength = 1.0f;
Recorder recorder;

static void sleep_seconds(double seconds) {
    if (seconds <= 0.0) {
        return;
    }

    struct timespec req;
    req.tv_sec = (time_t)seconds;
    req.tv_nsec = (long)((seconds - (double)req.tv_sec) * 1000000000.0);
    if (req.tv_nsec < 0) {
        req.tv_nsec = 0;
    }

    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    world.camera.width = width;
    world.camera.height = height;
    windowWidth = width;
    windowHeight = height;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera_process_scroll(&world.camera, yoffset);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        if (!middleMousePressed) {
            middleMousePressed = true;
            lastX = xpos;
            lastY = ypos;
        } else {
            float xoffset = xpos - lastX;
            float yoffset = ypos - lastY;
            lastX = xpos;
            lastY = ypos;
            
            camera_process_pan(&world.camera, xoffset, yoffset);
        }
    } else {
        middleMousePressed = false;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch (key) {
        case GLFW_KEY_H:
        case GLFW_KEY_F1:
            if (action == GLFW_PRESS) {
                hud_toggle(&world.hud);
            }
            break;
        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS && !spaceHeld) {
                savedAttractionStrength = particle_system_get_attraction_strength(world.particle_system);
                particle_system_set_attraction_strength(world.particle_system, 0.0f);
                spaceHeld = true;
            } else if (action == GLFW_RELEASE && spaceHeld) {
                particle_system_set_attraction_strength(world.particle_system, savedAttractionStrength);
                spaceHeld = false;
            }
            break;
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Create fullscreen window on primary monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        fprintf(stderr, "Failed to get primary monitor\n");
        glfwTerminate();
        return -1;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        fprintf(stderr, "Failed to get monitor video mode\n");
        glfwTerminate();
        return -1;
    }

    windowWidth = mode->width;
    windowHeight = mode->height;
    
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Particle Simulation", monitor, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Setup window
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    // Check OpenGL version and compute shader support
    int majorVersion, minorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    printf("OpenGL Version: %d.%d\n", majorVersion, minorVersion);

    // Initialize camera and world
    camera_init(&world.camera, windowWidth, windowHeight);

    ParticleSystem* ps = particle_system_create();
    if (!ps) {
        // Handle error
        return -1;
    }

    world_init(&world, window, ps);
    int targetFps = recorder_target_fps_from_env(60);
    recorder_init_from_env(&recorder, windowWidth, windowHeight, targetFps);

    // Add key callback
    glfwSetKeyCallback(window, key_callback);

    // Main loop
    const double targetFrameTime = 1.0 / (double)targetFps;
    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();

        static float lastFrame = 0.0f;
        double currentFrame = frameStart;
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update camera zoom and position
        camera_update(&world.camera, deltaTime);

        world_render(&world);
        recorder_capture_frame(&recorder);

        glfwSwapBuffers(window);
        
        glfwPollEvents();

        // Keep an explicit 60 FPS cap to improve frame pacing for capture stacks.
        double frameEnd = glfwGetTime();
        double remaining = targetFrameTime - (frameEnd - frameStart);
        sleep_seconds(remaining);
    }

    // Cleanup
    recorder_cleanup(&recorder);
    world_cleanup(&world);
    particle_system_destroy(ps);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
