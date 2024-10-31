#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

char* read_shader_file(const char* filename) {
    FILE* file;
    #ifdef _WIN32
        errno_t err = fopen_s(&file, filename, "rb");
        if (err != 0 || !file) {
            fprintf(stderr, "Failed to open shader file: %s\n", filename);
            return NULL;
        }
    #else
        file = fopen(filename, "rb");
        if (!file) {
            fprintf(stderr, "Failed to open shader file: %s\n", filename);
            return NULL;
        }
    #endif

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*)malloc(length + 1);
    size_t read = fread(content, 1, length, file);
    content[read] = '\0';

    // Clean the content: remove any non-ASCII characters
    size_t write = 0;
    for (size_t i = 0; i < read; i++) {
        if ((unsigned char)content[i] >= 32 && (unsigned char)content[i] <= 126) {
            content[write++] = content[i];
        } else if (content[i] == '\n' || content[i] == '\r' || content[i] == '\t') {
            content[write++] = content[i];
        }
    }
    content[write] = '\0';

    fclose(file);
    return content;
}

unsigned int compile_shader(const char* source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader compilation failed:\n%s\n", infoLog);
    }

    return shader;
}

void check_program_linking(unsigned int program, const char* type) {
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Program linking failed:\n%s\n", infoLog);
    }
} 