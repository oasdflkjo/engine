#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "camera.h"
#include "world.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

Camera camera;
World world;
float lastX = 0.0f;
float lastY = 0.0f;
bool middleMousePressed = false;
int windowWidth = 0;
int windowHeight = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera_process_scroll(&camera, yoffset);
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
            
            camera_process_pan(&camera, xoffset, yoffset);
        }
    } else {
        middleMousePressed = false;
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
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    // Create window
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    windowWidth = mode->width;
    windowHeight = mode->height;
    
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Particle Simulation", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // thanks for subscribing <3
    // it means a lot to me

    

    // Setup window
    glfwSetWindowPos(window, 0, 0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
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

    if (majorVersion < 4 || (majorVersion == 4 && minorVersion < 3)) {
        fprintf(stderr, "OpenGL 4.3 or higher is required for compute shaders\n");
        return -1;
    }

    // Initialize camera and world
    camera_init(&camera, windowWidth, windowHeight);
    world_init(&world);

    // Main loop
    const double targetFrameTime = 1.0 / 60.0;  // For 160 FPS
    
    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();
        
        // Get delta time
        static float lastFrame = 0.0f;
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera_reset(&camera);  // This will now trigger smooth transition
        }

        // Update camera zoom and position
        camera_update(&camera, deltaTime);

        world_render(&world, &camera);

        glfwSwapBuffers(window);

        // Add frame time measurement here
        static double lastRenderTime = 0.0;
        double currentTime = glfwGetTime();
        printf("Frame time: %.2f ms\n", (currentTime - lastRenderTime) * 1000.0);
        lastRenderTime = currentTime;
        
        glfwPollEvents();
        
        // Frame limiting
        while (glfwGetTime() - frameStart < targetFrameTime) {
            // Busy-wait or could use a sleep for better CPU usage
        }
    }

    // Cleanup
    world_cleanup(&world);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
