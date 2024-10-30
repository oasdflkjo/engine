#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2
#include <math.h>

// Grid shader sources
static const char* gridVertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "   gl_Position = projection * view * vec4(aPos, 1.0);\n"
    "}\0";

static const char* gridFragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vec4(0.5, 0.5, 0.5, 1.0);\n"
    "}\0";

// Helper function to create grid vertices
static void create_grid_vertices(float* vertices, int* vertexCount, float size, float spacing) {
    int index = 0;
    
    // Create horizontal lines
    for (float y = -size; y <= size; y += spacing) {
        vertices[index++] = -size;  // x1
        vertices[index++] = y;      // y1
        vertices[index++] = 0.0f;   // z1
        
        vertices[index++] = size;   // x2
        vertices[index++] = y;      // y2
        vertices[index++] = 0.0f;   // z2
    }
    
    // Create vertical lines
    for (float x = -size; x <= size; x += spacing) {
        vertices[index++] = x;      // x1
        vertices[index++] = -size;  // y1
        vertices[index++] = 0.0f;   // z1
        
        vertices[index++] = x;      // x2
        vertices[index++] = size;   // y2
        vertices[index++] = 0.0f;   // z2
    }
    
    *vertexCount = index / 3;
}

char* read_shader_file(const char* filename) {
    printf("Attempting to read shader file: %s\n", filename);
    FILE* file = fopen(filename, "rb");  // Open in binary mode
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("Shader file length: %ld bytes\n", length);

    char* content = (char*)malloc(length + 1);
    size_t read = fread(content, 1, length, file);
    content[read] = '\0';  // Null terminate at actual read length

    // Clean the content: remove any non-ASCII characters
    size_t write = 0;
    for (size_t i = 0; i < read; i++) {
        if ((unsigned char)content[i] >= 32 && (unsigned char)content[i] <= 126) {
            // Keep printable ASCII characters
            content[write++] = content[i];
        } else if (content[i] == '\n' || content[i] == '\r' || content[i] == '\t') {
            // Keep whitespace characters
            content[write++] = content[i];
        }
    }
    content[write] = '\0';  // Null terminate after cleaning

    printf("Cleaned shader content (%zu bytes):\n%s\n", write, content);

    fclose(file);
    return content;
}

static unsigned int compile_shader(const char* source, GLenum type) {
    const char* type_str = (type == GL_VERTEX_SHADER) ? "vertex" :
                          (type == GL_FRAGMENT_SHADER) ? "fragment" :
                          (type == GL_COMPUTE_SHADER) ? "compute" : "unknown";
    
    printf("Compiling %s shader...\n", type_str);
    
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "%s shader compilation failed:\n%s\n", type_str, infoLog);
        printf("Shader source:\n%s\n", source);  // Print the entire shader source
    } else {
        printf("%s shader compiled successfully\n", type_str);
    }

    return shader;
}

static void init_particles(World* world) {
    world->numParticles = MAX_PARTICLES;
    printf("Initializing %d particles...\n", world->numParticles);

    // Calculate grid dimensions for initial particle placement
    int grid_size = (int)sqrt(world->numParticles);
    float spacing = 20.0f / grid_size;  // 20.0f is our total width (-10 to +10)
    
    // Allocate aligned memory for positions and velocities
    vec2* positions = (vec2*)_aligned_malloc(world->numParticles * sizeof(vec2), 16);
    vec2* velocities = (vec2*)_aligned_malloc(world->numParticles * sizeof(vec2), 16);

    if (!positions || !velocities) {
        fprintf(stderr, "Failed to allocate aligned memory for particles!\n");
        _aligned_free(positions);
        _aligned_free(velocities);
        return;
    }

    // Pre-calculate some constants
    __m128 spacing_v = _mm_set1_ps(spacing);
    __m128 neg_ten = _mm_set1_ps(-10.0f);
    __m128 vel_scale = _mm_set1_ps(0.1f);
    
    // Process 4 particles at a time using SSE
    int aligned_count = (world->numParticles / 4) * 4;
    
    #pragma omp parallel for
    for (int i = 0; i < aligned_count; i += 4) {
        int x[4] = {(i) % grid_size, (i+1) % grid_size, (i+2) % grid_size, (i+3) % grid_size};
        int y[4] = {(i) / grid_size, (i+1) / grid_size, (i+2) / grid_size, (i+3) / grid_size};
        
        // Convert x indices to positions
        __m128 x_pos = _mm_add_ps(
            neg_ten,
            _mm_mul_ps(
                _mm_cvtepi32_ps(_mm_loadu_si128((__m128i*)x)),
                spacing_v
            )
        );
        
        // Convert y indices to positions
        __m128 y_pos = _mm_add_ps(
            neg_ten,
            _mm_mul_ps(
                _mm_cvtepi32_ps(_mm_loadu_si128((__m128i*)y)),
                spacing_v
            )
        );
        
        // Store positions
        _mm_store_ps((float*)&positions[i], x_pos);
        _mm_store_ps((float*)&positions[i] + 4, y_pos);
        
        // Generate random angles (0 to 2π)
        float angles[4] = {
            ((float)rand() / RAND_MAX) * 6.28318f,
            ((float)rand() / RAND_MAX) * 6.28318f,
            ((float)rand() / RAND_MAX) * 6.28318f,
            ((float)rand() / RAND_MAX) * 6.28318f
        };
        
        // Calculate velocities
        __m128 vx = _mm_mul_ps(_mm_set_ps(cosf(angles[3]), cosf(angles[2]), 
                                         cosf(angles[1]), cosf(angles[0])), vel_scale);
        __m128 vy = _mm_mul_ps(_mm_set_ps(sinf(angles[3]), sinf(angles[2]), 
                                         sinf(angles[1]), sinf(angles[0])), vel_scale);
        
        // Store velocities
        _mm_store_ps((float*)&velocities[i], vx);
        _mm_store_ps((float*)&velocities[i] + 4, vy);
    }
    
    // Handle remaining particles
    for (int i = aligned_count; i < world->numParticles; i++) {
        int x = i % grid_size;
        int y = i / grid_size;
        
        positions[i][0] = -10.0f + x * spacing;
        positions[i][1] = -10.0f + y * spacing;
        
        float angle = ((float)rand() / RAND_MAX) * 6.28318f;
        velocities[i][0] = cosf(angle) * 0.1f;
        velocities[i][1] = sinf(angle) * 0.1f;
    }

    // Create and initialize buffers
    glGenBuffers(1, &world->positionBuffer);
    glGenBuffers(1, &world->velocityBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world->positionBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, world->numParticles * sizeof(vec2), positions, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world->velocityBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, world->numParticles * sizeof(vec2), velocities, GL_DYNAMIC_DRAW);

    _aligned_free(positions);
    _aligned_free(velocities);

    printf("Particle buffers initialized successfully\n");

    // Create VAO for rendering
    glGenVertexArrays(1, &world->particleVAO);
    glBindVertexArray(world->particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, world->positionBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
    glEnableVertexAttribArray(0);
}

static void check_program_linking(unsigned int program, const char* type) {
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "%s program linking failed:\n%s\n", type, infoLog);
    } else {
        printf("%s program linked successfully\n", type);
    }
}

void world_init(World* world) {
    printf("Initializing world...\n");
    
    // Grid initialization
    world->gridSize = 10.0f;
    world->gridSpacing = 1.0f;
    
    // Calculate grid vertices
    int linesPerAxis = (2 * world->gridSize) / world->gridSpacing + 1;
    int totalLines = linesPerAxis * 2;
    int maxVertices = totalLines * 2 * 3;
    
    float* gridVertices = (float*)malloc(maxVertices * sizeof(float));
    int vertexCount;
    create_grid_vertices(gridVertices, &vertexCount, world->gridSize, world->gridSpacing);
    
    // Create and bind grid VAO and VBO
    glGenVertexArrays(1, &world->gridVAO);
    glGenBuffers(1, &world->gridVBO);
    
    glBindVertexArray(world->gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, world->gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), gridVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    free(gridVertices);
    
    // Compile grid shaders
    unsigned int gridVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gridVertexShader, 1, &gridVertexShaderSource, NULL);
    glCompileShader(gridVertexShader);
    
    unsigned int gridFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gridFragmentShader, 1, &gridFragmentShaderSource, NULL);
    glCompileShader(gridFragmentShader);
    
    world->gridShaderProgram = glCreateProgram();
    glAttachShader(world->gridShaderProgram, gridVertexShader);
    glAttachShader(world->gridShaderProgram, gridFragmentShader);
    glLinkProgram(world->gridShaderProgram);
    
    glDeleteShader(gridVertexShader);
    glDeleteShader(gridFragmentShader);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Initialize particle system
    world->mousePos[0] = 0.0f;
    world->mousePos[1] = 0.0f;
    world->deltaTime = 0.0f;

    printf("Loading compute shader...\n");
    char* computeSource = read_shader_file("shaders/particle.comp");
    if (!computeSource) {
        fprintf(stderr, "Failed to load compute shader!\n");
        return;
    }
    
    printf("Loading vertex shader...\n");
    char* vertexSource = read_shader_file("shaders/particle.vert");
    if (!vertexSource) {
        fprintf(stderr, "Failed to load vertex shader!\n");
        free(computeSource);
        return;
    }
    
    printf("Loading fragment shader...\n");
    char* fragmentSource = read_shader_file("shaders/particle.frag");
    if (!fragmentSource) {
        fprintf(stderr, "Failed to load fragment shader!\n");
        free(computeSource);
        free(vertexSource);
        return;
    }

    // Load and compile compute shader
    unsigned int computeShader = compile_shader(computeSource, GL_COMPUTE_SHADER);
    world->computeProgram = glCreateProgram();
    glAttachShader(world->computeProgram, computeShader);
    glLinkProgram(world->computeProgram);
    check_program_linking(world->computeProgram, "Compute");
    glDeleteShader(computeShader);
    free(computeSource);

    // Load and compile render shaders
    unsigned int vertexShader = compile_shader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compile_shader(fragmentSource, GL_FRAGMENT_SHADER);
    
    world->renderProgram = glCreateProgram();
    glAttachShader(world->renderProgram, vertexShader);
    glAttachShader(world->renderProgram, fragmentShader);
    glLinkProgram(world->renderProgram);
    check_program_linking(world->renderProgram, "Render");
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    free(vertexSource);
    free(fragmentSource);

    // Initialize particles
    init_particles(world);
}

void world_render(World* world, Camera* camera) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    world->deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Update particle positions
    world_update_particles(world);

    // Clear the buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from camera
    mat4 view;
    mat4 projection;
    camera_get_view_matrix(camera, view);
    camera_get_projection_matrix(camera, projection);
    
    // Render grid
    glUseProgram(world->gridShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(world->gridShaderProgram, "view"), 1, GL_FALSE, (float*)view);
    glUniformMatrix4fv(glGetUniformLocation(world->gridShaderProgram, "projection"), 1, GL_FALSE, (float*)projection);
    
    glBindVertexArray(world->gridVAO);
    int linesPerAxis = (2 * world->gridSize) / world->gridSpacing + 1;
    int totalLines = linesPerAxis * 2;
    glDrawArrays(GL_LINES, 0, totalLines * 2);

    // Render particles
    glUseProgram(world->renderProgram);
    glUniformMatrix4fv(glGetUniformLocation(world->renderProgram, "view"), 1, GL_FALSE, (float*)view);
    glUniformMatrix4fv(glGetUniformLocation(world->renderProgram, "projection"), 1, GL_FALSE, (float*)projection);

    glBindVertexArray(world->particleVAO);
    glDrawArrays(GL_POINTS, 0, world->numParticles);
}

void world_cleanup(World* world) {
    // Cleanup grid resources
    glDeleteVertexArrays(1, &world->gridVAO);
    glDeleteBuffers(1, &world->gridVBO);
    glDeleteProgram(world->gridShaderProgram);

    // Cleanup particle resources
    glDeleteVertexArrays(1, &world->particleVAO);
    glDeleteBuffers(1, &world->positionBuffer);
    glDeleteBuffers(1, &world->velocityBuffer);
    glDeleteProgram(world->computeProgram);
    glDeleteProgram(world->renderProgram);
}

void world_update_particles(World* world) {
    static int frame_count = 0;
    if (frame_count++ % 600 == 0) {  // Print every 600 frames
        printf("Updating particles: deltaTime=%f, mousePos=(%f, %f)\n", 
               world->deltaTime, world->mousePos[0], world->mousePos[1]);
    }
    
    glUseProgram(world->computeProgram);
    
    // Update uniforms
    glUniform1f(glGetUniformLocation(world->computeProgram, "delta_time"), world->deltaTime);
    glUniform2fv(glGetUniformLocation(world->computeProgram, "mouse_pos"), 1, world->mousePos);
    glUniform1i(glGetUniformLocation(world->computeProgram, "num_particles"), world->numParticles);

    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, world->positionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, world->velocityBuffer);

    // Dispatch compute shader
    int workGroupSize = 256;
    int numWorkGroups = (world->numParticles + workGroupSize - 1) / workGroupSize;
    glDispatchCompute(numWorkGroups, 1, 1);

    // Make sure compute shader finishes before rendering
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void world_set_mouse_pos(World* world, float x, float y) {
    world->mousePos[0] = x;
    world->mousePos[1] = y;
}