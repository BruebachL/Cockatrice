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

float luminance(vec3 col) {
    return dot(col, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec2 uv = qt_TexCoord0;
    vec4 base = texture(source, uv);

    float pos = fract(uv.x + gradientOffset * 2.0);
    vec4 grad;
    if (pos < 0.5) {
        float t = pos / 0.5;
        grad = mix(c0, c1, t);
    } else {
        float t = (pos - 0.5) / 0.5;
        grad = mix(c1, c2, t);
    }

    float lum = luminance(base.rgb);
    vec4 foil = vec4(grad.rgb * lum, grad.a * 0.9);

    vec2 center = vec2(highlightX, 0.5);
    float dist = distance(uv, center);
    float radial = smoothstep(0.3, 0.0, dist);
    vec4 highlight = vec4(1.0, 1.0, 1.0, radial * 0.22);

    float mask = 1.0;
    if (applyToArtOnly != 0) {
        if (uv.x < artRectNorm.x || uv.x > artRectNorm.x + artRectNorm.z ||
            uv.y < artRectNorm.y || uv.y > artRectNorm.y + artRectNorm.w) {
            mask = 0.0;
        }
    }

    vec3 blended = 1.0 - (1.0 - base.rgb) * (1.0 - (foil.rgb + highlight.rgb));
    float alpha = foil.a + highlight.a;

    vec3 outRgb = mix(base.rgb, blended, clamp(alpha, 0.0, 1.0) * mask);
    fragColor = vec4(outRgb, base.a) * qt_Opacity;
}
