#version 450 core

layout(binding = 0) uniform sampler2D source;

layout(std140, binding = 1) uniform FoilUniforms {
    float qt_Opacity;
    float gradientOffset;
    float highlightX;
    int applyToArtOnly;
    vec4 c0;
    vec4 c1;
    vec4 c2;
    vec4 artRectNorm; // x, y, w, h
};

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

// --- Utilities ---

vec3 hueRotate(vec3 color, float angle) {
    float s = sin(angle);
    float c = cos(angle);

    mat3 rot = mat3(
    vec3(0.299+0.701*c+0.168*s, 0.587-0.587*c+0.330*s, 0.114-0.114*c-0.497*s),
    vec3(0.299-0.299*c-0.328*s, 0.587+0.413*c+0.035*s, 0.114-0.114*c+0.292*s),
    vec3(0.299-0.3*c+1.25*s, 0.587-0.588*c-1.05*s, 0.114+0.886*c-0.203*s)
    );

    return clamp(rot * color, 0.0, 1.0);
}

float luminance(vec3 col) {
    return dot(col, vec3(0.299, 0.587, 0.114));
}

// --- Main ---

void main() {
    vec2 uv = qt_TexCoord0;
    vec4 base = texture(source, uv);

    // --- Rolling hue rotation using gradientOffset as time ---
    float t = gradientOffset * 6.2831 * 2.0;// full rotation
    float rollX = sin(uv.x * 6.2831 + t);
    float rollY = cos(uv.y * 6.2831 + t);
    float roll = (rollX + rollY) * 0.5;
    float lum = luminance(base.rgb);
    vec3 rotated = hueRotate(base.rgb, roll * 0.5);
    vec3 color = mix(base.rgb, rotated, 2.0 * lum);

    // --- Gradient foil ---
    float pos = fract(uv.x + gradientOffset * 2.0);
    vec4 grad;
    if (pos < 0.5) {
        float tGrad = pos / 0.5;
        grad = mix(c0, c1, tGrad);
    } else {
        float tGrad = (pos - 0.5) / 0.5;
        grad = mix(c1, c2, tGrad);
    }
    vec4 foil = vec4(grad.rgb * lum, grad.a * 0.9);

    // --- Radial highlight ---
    vec2 center = vec2(highlightX, 0.5);
    float dist = distance(uv, center);
    float radial = smoothstep(0.3, 0.0, dist);
    vec4 highlight = vec4(1.0, 1.0, 1.0, radial * 0.22);

    // --- Mask for art-only application ---
    float mask = 1.0;
    if (applyToArtOnly != 0) {
        if (uv.x < artRectNorm.x || uv.x > artRectNorm.x + artRectNorm.z ||
            uv.y < artRectNorm.y || uv.y > artRectNorm.y + artRectNorm.w) {
            mask = 0.0;
        }
    }

    // --- Blend everything ---
    vec3 blended = 1.0 - (1.0 - color) * (1.0 - (foil.rgb + highlight.rgb));
    float alpha = foil.a + highlight.a;
    vec3 outRgb = mix(color, blended, clamp(alpha, 0.0, 1.0) * mask);

    // --- Final output ---
    fragColor = vec4(outRgb, base.a) * qt_Opacity;

    // --- Dummy reads to ensure all uniforms are used (prevents ignored block warning) ---
    // This is optional but ensures no warnings even if some uniforms are optimized out
    vec4 _dummy = c0 + c1 + c2 + artRectNorm;
    float _maskDummy = float(applyToArtOnly) + highlightX + gradientOffset + qt_Opacity;
}
