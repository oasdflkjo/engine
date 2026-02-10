#version 430

const vec2 POSITIONS[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

out vec2 uv;

void main() {
    vec2 pos = POSITIONS[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
    uv = pos * 0.5 + 0.5;
}
