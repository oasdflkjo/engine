#version 430 core
in float velocity_magnitude;
out vec4 FragColor;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    // Miami Vice inspired color palette
    vec3 slow_color = vec3(0.0, 0.8, 0.8);    // Cyan/Turquoise
    vec3 mid_color = vec3(0.9, 0.0, 0.9);     // Hot Pink/Magenta
    vec3 fast_color = vec3(0.98, 0.2, 0.85);  // Electric Pink
    
    float min_velocity = 0.01;  // Lowered minimum velocity
    float max_velocity = 8.0;   // Increased maximum velocity
    
    // More aggressive logarithmic compression
    float normalized = clamp(velocity_magnitude / max_velocity, 0.0, 1.0);
    
    // Using log base 20 for more compression
    float log_normalized = 1.0 - log(1.0 + (1.0 - normalized) * 19.0) / log(20.0);
    
    // Apply additional power curve for even more compression
    log_normalized = pow(log_normalized, 0.7);
    
    // Three-step interpolation with adjusted thresholds
    vec3 final_color;
    if (log_normalized < 0.4) {  // Increased first threshold
        final_color = mix(slow_color, mid_color, log_normalized * 2.5);  // Adjusted multiplier
    } else if (log_normalized < 0.7) {  // Increased second threshold
        final_color = mix(mid_color, fast_color, (log_normalized - 0.4) * 3.33);  // Adjusted multiplier
    } else {
        final_color = fast_color;
    }
    
    // Enhanced glow effect
    float brightness = 0.85 + (log_normalized * 0.35);
    final_color *= brightness;
    
    // Slightly stronger white glow
    final_color = mix(final_color, vec3(1.0), log_normalized * 0.2);
    
    FragColor = vec4(final_color, 1.0);
}