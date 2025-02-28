#ifndef POST_PROCESSOR_H
#define POST_PROCESSOR_H

#include <glad/glad.h>
#include "shader_program.h"

typedef struct {
    GLuint framebuffer;
    GLuint textureColorBuffer;
    GLuint rbo;
    GLuint quadVAO;
    GLuint quadVBO;
    ShaderProgram* shader;
    int width;
    int height;
} PostProcessor;

void post_processor_init(PostProcessor* pp, int width, int height);
void post_processor_begin(PostProcessor* pp);
void post_processor_end(PostProcessor* pp);
void post_processor_render(PostProcessor* pp, float time);
void post_processor_cleanup(PostProcessor* pp);
void post_processor_resize(PostProcessor* pp, int width, int height);

#endif // POST_PROCESSOR_H