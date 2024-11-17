#include "shader_program.h"
#include "shader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ShaderProgram* shader_program_create(const char** shader_files, GLenum* shader_types, int shader_count) {
    ShaderProgram* program = malloc(sizeof(ShaderProgram));
    if (!program) {
        return NULL;
    }

    program->program = glCreateProgram();
    program->uniform_locations = NULL;
    program->uniform_names = NULL;
    program->uniform_count = 0;

    // Compile and attach all shaders
    for (int i = 0; i < shader_count; i++) {
        char* source = read_shader_file(shader_files[i]);
        if (!source) {
            shader_program_destroy(program);
            return NULL;
        }

        GLuint shader = compile_shader(source, shader_types[i]);
        free(source);

        if (!shader) {
            shader_program_destroy(program);
            return NULL;
        }

        glAttachShader(program->program, shader);
        glDeleteShader(shader);  // Mark for deletion (will be deleted when program is deleted)
    }

    // Link program
    glLinkProgram(program->program);
    
    // Check linking status
    GLint success;
    glGetProgramiv(program->program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program->program, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Shader program linking failed: %s\n", infoLog);
        shader_program_destroy(program);
        return NULL;
    }

    return program;
}

void shader_program_destroy(ShaderProgram* program) {
    if (program) {
        if (program->program) {
            glDeleteProgram(program->program);
        }
        free(program->uniform_locations);
        free(program->uniform_names);
        free(program);
    }
}

void shader_program_cache_uniforms(ShaderProgram* program, const char** uniform_names, int count) {
    free(program->uniform_locations);
    free(program->uniform_names);

    program->uniform_count = count;
    program->uniform_locations = malloc(count * sizeof(GLint));
    program->uniform_names = malloc(count * sizeof(char*));

    for (int i = 0; i < count; i++) {
        program->uniform_names[i] = uniform_names[i];
        program->uniform_locations[i] = glGetUniformLocation(program->program, uniform_names[i]);
    }
}

GLint shader_program_get_uniform(ShaderProgram* program, const char* name) {
    // First check cached uniforms
    for (int i = 0; i < program->uniform_count; i++) {
        if (strcmp(program->uniform_names[i], name) == 0) {
            return program->uniform_locations[i];
        }
    }
    // If not found in cache, get from OpenGL
    return glGetUniformLocation(program->program, name);
}

void shader_program_use(ShaderProgram* program) {
    glUseProgram(program->program);
}

void shader_program_set_float(ShaderProgram* program, const char* name, float value) {
    GLint location = shader_program_get_uniform(program, name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void shader_program_set_int(ShaderProgram* program, const char* name, int value) {
    GLint location = shader_program_get_uniform(program, name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void shader_program_set_vec2(ShaderProgram* program, const char* name, float x, float y) {
    GLint location = shader_program_get_uniform(program, name);
    if (location != -1) {
        float values[2] = {x, y};
        glUniform2fv(location, 1, values);
    }
}

void shader_program_set_mat4(ShaderProgram* program, const char* name, float* value) {
    GLint location = shader_program_get_uniform(program, name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, value);
    }
} 