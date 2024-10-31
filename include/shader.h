#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

// Function declarations
char* read_shader_file(const char* filename);
unsigned int compile_shader(const char* source, GLenum type);
void check_program_linking(unsigned int program, const char* type);

#endif // SHADER_H 