#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <stdbool.h>

// Forward declare OpenGL types to avoid include conflicts
typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;

typedef struct {
    GLuint program;
    GLint* uniform_locations;
    const char** uniform_names;
    int uniform_count;
} ShaderProgram;

// Core functions
ShaderProgram* shader_program_create(const char** shader_files, GLenum* shader_types, int shader_count);
void shader_program_destroy(ShaderProgram* program);

// Uniform handling
void shader_program_cache_uniforms(ShaderProgram* program, const char** uniform_names, int count);
GLint shader_program_get_uniform(ShaderProgram* program, const char* name);
void shader_program_use(ShaderProgram* program);

// Uniform setters
void shader_program_set_float(ShaderProgram* program, const char* name, float value);
void shader_program_set_int(ShaderProgram* program, const char* name, int value);
void shader_program_set_vec2(ShaderProgram* program, const char* name, float x, float y);
void shader_program_set_mat4(ShaderProgram* program, const char* name, float* value);

#endif // SHADER_PROGRAM_H 