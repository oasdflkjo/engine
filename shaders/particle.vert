#version 430 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in float aVelocityMag;

uniform mat4 projection;
uniform mat4 view;

out float velocity_magnitude;

void main() {
    vec2 position = aPos;
    float vel_mag = aVelocityMag;
    
    gl_Position = projection * view * vec4(position, 0.0, 1.0);
    gl_PointSize = 2.0;
    
    velocity_magnitude = vel_mag;
}