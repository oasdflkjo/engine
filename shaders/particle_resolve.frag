#version 430

in vec2 uv;
out vec4 FragColor;

uniform usampler2D density_tex;
uniform float pixels_per_world;
uniform float particle_radius_world;

void main() {
    uint d = texture(density_tex, uv).r;
    if (d == 0u) {
        FragColor = vec4(0.0);
        return;
    }

    float density = float(d);

    // Log-compress the dynamic range so high-density regions do not immediately saturate.
    float t = clamp(log2(1.0 + density) / 10.0, 0.0, 1.0);

    // Multi-stop palette to avoid single-color dominance.
    vec3 c0 = vec3(0.04, 0.08, 0.22); // deep blue
    vec3 c1 = vec3(0.10, 0.55, 0.95); // cyan-blue
    vec3 c2 = vec3(0.66, 0.24, 0.90); // violet
    vec3 c3 = vec3(1.00, 0.52, 0.72); // soft pink
    vec3 c4 = vec3(1.00, 0.94, 0.82); // warm highlight

    vec3 color;
    if (t < 0.30) {
        color = mix(c0, c1, smoothstep(0.00, 0.30, t));
    } else if (t < 0.62) {
        color = mix(c1, c2, smoothstep(0.30, 0.62, t));
    } else if (t < 0.88) {
        color = mix(c2, c3, smoothstep(0.62, 0.88, t));
    } else {
        color = mix(c3, c4, smoothstep(0.88, 1.00, t));
    }

    float alpha = clamp(0.08 + pow(t, 0.85) * 0.92, 0.0, 1.0);

    FragColor = vec4(color * alpha, alpha);
}
