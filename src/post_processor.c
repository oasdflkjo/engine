#include "post_processor.h"
#include <stdlib.h>

static void create_framebuffer(PostProcessor* pp) {
    // Generate framebuffer
    glGenFramebuffers(1, &pp->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, pp->framebuffer);

    // Create color texture
    glGenTextures(1, &pp->textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, pp->textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, pp->width, pp->height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pp->textureColorBuffer, 0);

    // Create render buffer object
    glGenRenderbuffers(1, &pp->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, pp->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, pp->width, pp->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pp->rbo);
}

static void create_quad(PostProcessor* pp) {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &pp->quadVAO);
    glGenBuffers(1, &pp->quadVBO);
    
    glBindVertexArray(pp->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pp->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void post_processor_init(PostProcessor* pp, int width, int height) {
    pp->width = width;
    pp->height = height;

    // Create shader program
    const char* shader_files[] = {
        "shaders/post.vert",
        "shaders/post.frag"
    };
    GLenum shader_types[] = {
        GL_VERTEX_SHADER,
        GL_FRAGMENT_SHADER
    };
    pp->shader = shader_program_create(shader_files, shader_types, 2);

    create_framebuffer(pp);
    create_quad(pp);
}

void post_processor_begin(PostProcessor* pp) {
    glBindFramebuffer(GL_FRAMEBUFFER, pp->framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void post_processor_end(PostProcessor* pp) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void post_processor_render(PostProcessor* pp, float time) {
    shader_program_use(pp->shader);
    shader_program_set_float(pp->shader, "time", time);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pp->textureColorBuffer);
    
    glBindVertexArray(pp->quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void post_processor_cleanup(PostProcessor* pp) {
    glDeleteFramebuffers(1, &pp->framebuffer);
    glDeleteTextures(1, &pp->textureColorBuffer);
    glDeleteRenderbuffers(1, &pp->rbo);
    glDeleteVertexArrays(1, &pp->quadVAO);
    glDeleteBuffers(1, &pp->quadVBO);
    shader_program_destroy(pp->shader);
}

void post_processor_resize(PostProcessor* pp, int width, int height) {
    pp->width = width;
    pp->height = height;
    
    glBindTexture(GL_TEXTURE_2D, pp->textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    
    glBindRenderbuffer(GL_RENDERBUFFER, pp->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}