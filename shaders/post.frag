#version 430
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float time;

void main() {
    vec2 uv = TexCoords;
    
    // Add slight chromatic aberration
    float offset = 0.002;
    vec4 r = texture(screenTexture, vec2(uv.x + offset, uv.y));
    vec4 g = texture(screenTexture, uv);
    vec4 b = texture(screenTexture, vec2(uv.x - offset, uv.y));
    
    // Create a subtle glow effect
    vec2 resolution = textureSize(screenTexture, 0);
    float glow = 0.0;
    float radius = 2.0;
    int samples = 8;
    
    for(int i = 0; i < samples; i++) {
        float angle = 2.0 * 3.14159 * float(i) / float(samples);
        vec2 offset = vec2(cos(angle), sin(angle)) * radius / resolution;
        glow += texture(screenTexture, uv + offset).a;
    }
    glow /= float(samples);
    
    // Combine effects
    vec4 color = vec4(r.r, g.g, b.b, 1.0);
    color += vec4(0.3, 0.4, 0.5, 1.0) * glow * 0.5;
    
    // Add subtle vignette
    vec2 center = uv - 0.5;
    float vignette = 1.0 - dot(center, center) * 0.5;
    color *= vignette;
    
    FragColor = color;
}