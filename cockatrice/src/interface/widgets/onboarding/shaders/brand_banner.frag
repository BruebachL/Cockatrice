#version 440

// ════════════════════════════════════════════════════════════════════════
// brand_banner.frag
//
// One shader, six "motifs" (uMode 0..5), all built on top of the SAME
// shared backgroundField() -- a dark neutral gradient with two layered,
// slow-moving flow-noise fields for depth, identical on every page. Only
// the foreground emphasis differs per motif, and it's kept deliberately
// small: the traced brand mark drawn in QML (BrandBanner.qml) is the one
// real "moment" on every page, so these motifs stay quiet texture.
//
// IMPORTANT: the uniform block below must list custom uniforms in EXACTLY
// the order they're declared as properties on each ShaderEffect instance in
// BrandBanner.qml (after the two Qt-supplied members, qt_Matrix/qt_Opacity).
// A mismatch compiles fine and silently scrambles which QML property
// drives which uniform.
// ════════════════════════════════════════════════════════════════════════

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    mat4 qt_Matrix;
    float qt_Opacity;
    float iTime;
    float uAspect;
    float uMode;
    float uSpeed;
    float uSeed;
    vec4 uColorA;
    vec4 uColorB;
    vec4 uAccent;
};

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p)
{
    float v = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 3; i++) {
        v += amp * valueNoise(p);
        p *= 2.03;
        amp *= 0.5;
    }
    return v;
}

// Domain-warped flow noise: sampling fbm at a position that is itself
// perturbed by fbm gives motion a liquid, flowing quality rather than
// noise simply sliding under a fixed shape.
float flowNoise(vec2 p, float t)
{
    vec2 warp1 = vec2(fbm(p + vec2(0.0, 0.0)), fbm(p + vec2(5.2, 1.3)));
    vec2 warp2 = vec2(fbm(p + 4.0 * warp1 + vec2(1.7, 9.2) + t * 0.6),
                      fbm(p + 4.0 * warp1 + vec2(8.3, 2.8) - t * 0.5));
    return fbm(p + 4.0 * warp2 + t * 0.15);
}

// Dual falloff -- tight bright core plus a soft wide halo -- reads as a
// real glow instead of a flat disc. Radii are in UV units (0..1 across the
// banner), tuned per call site.
float bloom(float d, float coreRadius, float haloRadius)
{
    float core = exp(-(d * d) / (coreRadius * coreRadius));
    float halo = exp(-d / haloRadius) * 0.4;
    return core + halo;
}

// Rounded-box SDF (Inigo Quilez). p is local space (uv - shape center).
float roundedBoxSDF(vec2 p, vec2 halfSize, float radius)
{
    vec2 d = abs(p) - halfSize + radius;
    return length(max(d, 0.0)) - radius + min(max(d.x, d.y), 0.0);
}

// Shared dark-neutral background: one eased gradient, two independent
// flow-noise layers at different scale/speed (fakes depth/parallax without
// needing real layer separation), identical on every page.
vec3 backgroundField(vec2 uv, float time)
{
    float baseD = smoothstep(0.0, 1.0, uv.y * 0.5 + uv.x * 0.3);
    float painted = flowNoise(uv * 1.3, time * 0.05) - 0.5;
    baseD = clamp(baseD + painted * 0.16, 0.0, 1.0);
    vec3 col = mix(uColorA.rgb, uColorB.rgb, baseD);

    float deep = fbm(uv * 1.1 + vec2(37.1, 12.4) + time * 0.015);
    col = mix(col, uColorB.rgb, (deep - 0.5) * 0.10);

    float mid = fbm(uv * 2.3 + vec2(4.3, 88.0) + time * 0.05);
    col = mix(col, uColorA.rgb, (mid - 0.5) * 0.07);

    return col;
}

float vignette(vec2 uv)
{
    vec2 c = uv - 0.5;
    c.x *= max(uAspect, 0.0001);
    return smoothstep(1.0, 0.25, length(c));
}

// ── Motifs ─────────────────────────────────────────────────────────────
// All deliberately quiet -- the traced brand mark drawn in QML carries the
// "moment" on every page now, so these stay background texture.

vec3 motifWelcome(vec2 uv, vec3 bg, float t)
{
    return bg;
}

vec3 motifCardDatabase(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;
    const int CARDS = 4;
    for (int i = 0; i < CARDS; i++) {
        float fi = float(i);
        float depth = fract(fi * 0.618 + uSeed);
        float speed = mix(0.05, 0.02, depth);
        float cycle = fract(t * speed + fi * 0.31 + uSeed);
        vec2 pos = vec2(mix(-0.2, 1.2, cycle), 0.5 + sin(cycle * 6.283 * 1.4 + fi * 2.0) * 0.26);
        float scale = mix(0.15, 0.07, depth);

        vec2 p = uv - pos;
        float d = roundedBoxSDF(p, vec2(scale * 0.7, scale), scale * 0.25);
        float shape = smoothstep(0.04, -0.03, d);
        float brightness = mix(0.55, 1.0, 1.0 - depth);

        col = mix(col, uColorB.rgb * 1.2, shape * brightness * 0.2);
        col += uAccent.rgb * smoothstep(0.03, 0.0, abs(d)) * 0.05 * brightness;
    }
    return col;
}

vec3 motifTheming(vec2 uv, vec3 bg, float t)
{
    float flow = flowNoise(uv * 1.7 + vec2(uSeed * 3.0, 5.0), t * 0.15);
    vec3 col = mix(bg, uColorB.rgb * 1.1, (flow - 0.5) * 0.3);
    float crest = smoothstep(0.82, 0.97, flow);
    return col + uAccent.rgb * crest * 0.1;
}

vec3 motifAccount(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;
    vec2 origin = vec2(0.5, 0.52);
    const int BODIES = 3;
    for (int i = 0; i < BODIES; i++) {
        float fi = float(i);
        float radius = mix(0.14, 0.24, fract(fi * 0.53 + uSeed));
        float speed = mix(0.05, 0.09, fract(fi * 0.71));
        float angle = t * speed + fi * 2.4 + uSeed * 6.0;
        vec2 pos = origin + vec2(cos(angle), sin(angle) * 0.6) * radius;

        // Small, tight points of light -- not a soft field filling the frame.
        col += uAccent.rgb * bloom(length(uv - pos), 0.012, 0.05) * 0.5;
    }
    return col;
}

vec3 motifPreferences(vec2 uv, vec3 bg, float t)
{
    vec2 cell = fract(uv * 12.0) - 0.5;
    float gridLine = 1.0 - smoothstep(0.0, 0.05, min(abs(cell.x), abs(cell.y)));
    vec3 col = bg + uAccent.rgb * gridLine * 0.03;

    float sweepCoord = fract((uv.x + uv.y) * 0.5 - t * 0.1);
    float sweepDist = min(sweepCoord, 1.0 - sweepCoord);
    col += uAccent.rgb * smoothstep(0.012, 0.0, sweepDist);
    col += uAccent.rgb * smoothstep(0.06, 0.0, sweepDist) * 0.25;
    return col;
}

vec3 motifFinish(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg * 1.02;
    float sweepPos = fract(t * 0.05);
    float sweepDist = abs((uv.x * 0.6 + uv.y * 0.4) - sweepPos);
    col += uAccent.rgb * bloom(sweepDist, 0.025, 0.09) * 0.16;
    return col;
}

void main()
{
    vec2 uv = qt_TexCoord0;
    float t = iTime * uSpeed;

    vec3 bg = backgroundField(uv, iTime);

    vec3 col;
    if (uMode < 0.5) col = motifWelcome(uv, bg, t);
    else if (uMode < 1.5) col = motifCardDatabase(uv, bg, t);
    else if (uMode < 2.5) col = motifTheming(uv, bg, t);
    else if (uMode < 3.5) col = motifAccount(uv, bg, t);
    else if (uMode < 4.5) col = motifPreferences(uv, bg, t);
    else col = motifFinish(uv, bg, t);

    col *= mix(0.62, 1.0, vignette(uv));
    fragColor = vec4(col, 1.0) * qt_Opacity;
}