#version 430

in vec4 particleColor;
out vec4 FragColor;

void main() {
    // Calculate distance from center of point sprite
    vec2 coord = gl_PointCoord - vec2(0.5);
    float r = length(coord) * 2.0;
    
    // Smooth circular particle
    float alpha = smoothstep(1.0, 0.0, r);
    
    // Add Miami Vice style colors
    vec3 color = particleColor.rgb;
    float brightness = max(max(color.r, color.g), color.b);
    
    // Hot pink and cyan colors
    vec3 hotPink = vec3(1.0, 0.1, 0.8);    // More vibrant pink
    vec3 babyBlue = vec3(0.2, 0.8, 1.0);   // Bright cyan/baby blue
    
    // Adjusted threshold to favor blue more
    // Now blue will be present up until higher brightness levels
    vec3 miamiColor = mix(babyBlue, hotPink, smoothstep(0.5, 0.9, brightness));
    
    // Increased blue presence in the final mix
    color = mix(babyBlue, miamiColor, smoothstep(0.2, 0.8, brightness));
    
    FragColor = vec4(color, particleColor.a * alpha);
}