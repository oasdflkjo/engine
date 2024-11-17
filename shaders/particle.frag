#version 430

in vec4 particleColor;
out vec4 FragColor;

layout(std430, binding = 3) buffer ColorLUT {
    vec4 colorLUT[];  // RGB color array stored in GPU memory
};

void main() {
    // Calculate distance from center of point sprite
    vec2 coord = gl_PointCoord - vec2(0.5);
    float r = length(coord) * 2.0;
    
    // Smooth circular particle
    float alpha = smoothstep(1.0, 0.0, r);
    
    // Get color from velocity-based brightness using LUT
    float brightness = max(max(particleColor.r, particleColor.g), particleColor.b);
    int lutIndex = int(brightness * 255.0);  // Map 0-1 to 0-255
    vec3 color = colorLUT[lutIndex].rgb;
    
    FragColor = vec4(color, alpha);
}