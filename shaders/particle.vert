#version 430 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in float aVelocityMag;

uniform mat4 projection;
uniform mat4 view;

out float velocity_magnitude;
out float should_render;

void main() {
    vec4 viewPos = view * vec4(aPos, 0.0, 1.0);
    gl_Position = projection * viewPos;
    
    float dist = length(viewPos.xyz);
    float screenSize = 2.0 / dist;
    
    vec2 screenPos = gl_Position.xy / gl_Position.w;
    float gridSize = 0.005;
    vec2 gridPos = floor(screenPos / gridSize);
    
    uint particleID = gl_VertexID;
    float random = fract(sin(dot(gridPos, vec2(12.9898, 78.233)) + float(particleID)) * 43758.5453);
    
    should_render = random > 0.5 ? 1.0 : 0.0;
    
    float baseSize = 2.0;
    float velocityScale = clamp(aVelocityMag * 0.5, 0.5, 2.0);
    gl_PointSize = baseSize * velocityScale * screenSize;
    
    velocity_magnitude = aVelocityMag;
}