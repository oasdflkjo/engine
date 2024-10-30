#version 430 core
layout (location = 0) in vec2 position;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(position, 0.0, 1.0);
    gl_PointSize = 2.0;
}