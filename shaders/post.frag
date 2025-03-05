#version 430
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float time;

// Character rendering function for ASCII effect
float character(int n, vec2 p)
{
    p = floor(p*vec2(-4.0, 4.0) + 2.5);
    if (clamp(p.x, 0.0, 4.0) == p.x)
    {
        if (clamp(p.y, 0.0, 4.0) == p.y)    
        {
            int a = int(round(p.x) + 5.0 * round(p.y));
            if (((n >> a) & 1) == 1) return 1.0;
        }    
    }
    return 0.0;
}

// Direct character selection based on brightness
int getCharacterDirect(float brightness) {
    // Base characters
    int chars[9] = int[](
        0,         // Empty (for true black)
        4096,      // . (very dark)
        65600,     // :
        163153,    // *
        15255086,  // o
        13121101,  // &
        15252014,  // 8
        13195790,  // @
        11512810   // # (brightest)
    );
    
    // Select character based on brightness
    int index = int(brightness * 8.0);
    index = clamp(index, 0, 8);
    return chars[index];
}

// Extreme color enhancement function for YouTube
vec3 enhanceColor(vec3 color) {
    // Dramatically increase saturation
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    vec3 saturated = mix(vec3(luminance), color, 2.5); // Boost saturation by 150%
    
    // Increase contrast significantly
    vec3 contrasted = (saturated - 0.5) * 1.8 + 0.5; // Boost contrast by 80%
    
    // Add strong color shift for more vibrance
    float r = contrasted.r * 1.3;  // Boost reds by 30%
    float g = contrasted.g * 1.2;  // Boost greens by 20%
    float b = contrasted.b * 1.5;  // Boost blues by 50%
    
    // Add a slight color tint to make it more interesting
    vec3 tinted = vec3(
        r + 0.05,  // Add a bit of red
        g + 0.02,  // Add a tiny bit of green
        b + 0.1    // Add more blue
    );
    
    return clamp(tinted, 0.0, 1.0);
}

void main() {
    vec2 uv = TexCoords;
    vec2 resolution = textureSize(screenTexture, 0);
    
    // Fixed character size (no breathing)
    float charSize = 10.0; // Smaller characters for more detail
    
    // Calculate pixel position and character cell
    vec2 pix = uv * resolution;
    vec2 charCell = floor(pix / charSize);
    vec2 within = mod(pix, charSize) / charSize;
    
    // Sample the original texture at the center of the character cell
    vec2 cellUV = (charCell + vec2(0.5)) * charSize / resolution;
    vec4 originalColor = texture(screenTexture, cellUV);
    
    // Enhance the colors dramatically
    vec3 enhancedColor = enhanceColor(originalColor.rgb);
    
    // Calculate brightness from enhanced color
    float brightness = length(enhancedColor) / 1.732; // Normalize by sqrt(3)
    
    // Get character based on brightness directly
    int n = getCharacterDirect(brightness);
    
    // Calculate character position within the cell
    vec2 charPos = within * 2.0 - vec2(1.0);
    
    // Apply character to color (no pulsing)
    float char = character(n, charPos);
    
    // For very dark areas, make them truly black
    float darkThreshold = 0.1;
    if (brightness < darkThreshold) {
        // Smoothly fade to black for very dark areas
        float darkFactor = smoothstep(0.0, darkThreshold, brightness);
        char *= darkFactor;
    }
    
    // Apply character to enhanced color
    vec3 asciiColor = enhancedColor * char;
    
    // Add a strong colored glow to bright characters
    if (brightness > 0.4 && char > 0.5) {
        // Add a colored rim to bright characters
        vec3 glowColor = enhancedColor * 1.5;
        asciiColor = mix(asciiColor, glowColor, 0.4);
    }
    
    // Add a subtle bloom effect
    vec3 bloomColor = enhancedColor * 1.8;
    float bloomAmount = smoothstep(0.4, 0.8, brightness) * 0.3;
    asciiColor = mix(asciiColor, bloomColor, bloomAmount * char);
    
    // Final color with ASCII effect
    vec4 finalColor = vec4(asciiColor, 1.0);
    
    // Add dramatic vignette with colored edges
    vec2 center = uv - 0.5;
    float vignetteAmount = dot(center, center) * 0.7; // Stronger vignette
    float vignette = 1.0 - vignetteAmount;
    
    // Add vibrant colored vignette edges
    vec3 vignetteColor = vec3(1.0, 0.3, 0.8); // Vibrant pink/purple edge
    finalColor.rgb = mix(finalColor.rgb, finalColor.rgb * vignetteColor, smoothstep(0.0, 0.6, vignetteAmount) * 0.6);
    
    // Apply vignette darkening
    finalColor.rgb *= vignette;
    
    // Final color boost for YouTube
    finalColor.rgb = pow(finalColor.rgb, vec3(0.85)); // Gamma adjustment to brighten
    
    FragColor = finalColor;
}