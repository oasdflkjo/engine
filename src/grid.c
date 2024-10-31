#include "grid.h"
#include <stdio.h>
#include <stdlib.h>

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

void grid_init(Grid* grid, float size, float spacing) {
    grid->size = size;
    grid->spacing = spacing;
    
    // Calculate grid vertices
    int linesPerAxis = (2 * size) / spacing + 1;
    int totalLines = linesPerAxis * 2;
    int maxVertices = totalLines * 2 * 3;
    
    float* gridVertices = (float*)malloc(maxVertices * sizeof(float));
    int vertexCount;
    create_grid_vertices(gridVertices, &vertexCount, size, spacing);
    
    // Create and bind grid VAO and VBO
    glGenVertexArrays(1, &grid->VAO);
    glGenBuffers(1, &grid->VBO);
    
    glBindVertexArray(grid->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, grid->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), gridVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    free(gridVertices);
    
    // Compile grid shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &gridVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &gridFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    grid->shaderProgram = glCreateProgram();
    glAttachShader(grid->shaderProgram, vertexShader);
    glAttachShader(grid->shaderProgram, fragmentShader);
    glLinkProgram(grid->shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void grid_render(Grid* grid, float* view, float* projection) {
    glUseProgram(grid->shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(grid->shaderProgram, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(grid->shaderProgram, "projection"), 1, GL_FALSE, projection);
    
    glBindVertexArray(grid->VAO);
    int linesPerAxis = (2 * grid->size) / grid->spacing + 1;
    int totalLines = linesPerAxis * 2;
    glDrawArrays(GL_LINES, 0, totalLines * 2);
}

void grid_cleanup(Grid* grid) {
    glDeleteVertexArrays(1, &grid->VAO);
    glDeleteBuffers(1, &grid->VBO);
    glDeleteProgram(grid->shaderProgram);
} 