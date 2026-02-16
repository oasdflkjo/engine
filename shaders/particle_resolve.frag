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
    float ppw = max(pixels_per_world, 0.001);

    // Continuous zoom compensation:
    // when zooming in (higher pixels_per_world), per-pixel hits become sparser,
    // so boost effective density smoothly instead of switching render modes.
    float zoomCompensation = clamp((ppw * ppw) * 0.22, 0.06, 20.0);
    float effectiveDensity = density * zoomCompensation;

    // Log-compress the dynamic range so high-density regions do not immediately saturate.
    float t = clamp(log2(1.0 + effectiveDensity) / 12.0, 0.0, 1.0);

    // Multi-stop palette to avoid single-color dominance.
    vec3 c0 = vec3(0.03, 0.07, 0.25); // deeper blue
    vec3 c1 = vec3(0.08, 0.50, 0.95); // blue-cyan
    vec3 c2 = vec3(0.66, 0.24, 0.90); // violet
    vec3 c3 = vec3(1.00, 0.52, 0.72); // soft pink
    vec3 c4 = vec3(1.00, 0.94, 0.82); // warm highlight

    vec3 color;
    if (t < 0.36) {
        color = mix(c0, c1, smoothstep(0.00, 0.36, t));
    } else if (t < 0.66) {
        color = mix(c1, c2, smoothstep(0.36, 0.66, t));
    } else if (t < 0.90) {
        color = mix(c2, c3, smoothstep(0.66, 0.90, t));
    } else {
        color = mix(c3, c4, smoothstep(0.90, 1.00, t));
    }

    // Slightly stronger burn in higher densities without blowing out low-density detail.
    float burnBoost = smoothstep(0.62, 1.0, t) * 0.10;
    float alpha = clamp(0.06 + pow(t, 1.02) * 0.82 + burnBoost, 0.0, 0.88);

    FragColor = vec4(color * alpha, alpha);
}
