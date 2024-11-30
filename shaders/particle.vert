#version 460

// Input from current render buffer
layout(std430, binding = 0) buffer RenderPositions {
    vec2 positions[];
};

uniform mat4 view;
uniform mat4 projection;

void main() {
    // Read position from the current render buffer
    vec2 pos = positions[gl_VertexID];
    gl_Position = projection * view * vec4(pos.x, pos.y, 0.0, 1.0);
    gl_PointSize = 2.0;
}