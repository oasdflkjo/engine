#version 430 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in float aVelocityMag;

uniform mat4 projection;
uniform mat4 view;

out float velocity_magnitude;
out float should_render;  // We'll keep this but always set it to 1.0

void main() {
    vec4 viewPos = view * vec4(aPos, 0.0, 1.0);
    gl_Position = projection * viewPos;
    
    // Always render the particle
    should_render = 1.0;
    
    // Keep the distance-based size scaling
    float dist = length(viewPos.xyz);
    float screenSize = 2.0 / dist;
    float baseSize = 2.0;
    float velocityScale = clamp(aVelocityMag * 0.5, 0.5, 2.0);
    gl_PointSize = baseSize * velocityScale * screenSize;
    
    velocity_magnitude = aVelocityMag;
}