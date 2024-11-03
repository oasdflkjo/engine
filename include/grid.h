#ifndef GRID_H
#define GRID_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

typedef struct {
    unsigned int VAO;
    unsigned int VBO;
    unsigned int shaderProgram;
    float size;
    float spacing;
} Grid;

void grid_init(Grid* grid, float size, float spacing);
void grid_render(Grid* grid, float* view, float* projection);
void grid_cleanup(Grid* grid);

#endif // GRID_H 