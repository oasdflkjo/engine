#version 430

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 velocity;

uniform mat4 view;
uniform mat4 projection;
uniform float pixels_per_world;
uniform float particle_radius_world;

out vec4 particleColor;

void main() {
    gl_Position = projection * view * vec4(position, 0.0, 1.0);
    
    // Keep particle size consistent in world space; avoids extreme overdraw when zoomed out
    float pointSize = 2.0 * particle_radius_world * pixels_per_world;
    gl_PointSize = clamp(pointSize, 1.0, 8.0);
    
    // Calculate velocity magnitude
    float speed = length(velocity);
    
    // Use logarithmic scaling for better color distribution
    float normalizedSpeed = log(1.0 + speed) / log(1.0 + 50.0); // 50.0 is max expected speed
    
    // Create a color gradient from cyan (slow) to pink (fast)
    vec3 color = mix(
        vec3(0.0, 0.8, 0.8),  // Cyan for slow particles
        vec3(0.9, 0.2, 0.5),  // Pink for fast particles
        normalizedSpeed
    );
    
    particleColor = vec4(color, 1.0);
}
