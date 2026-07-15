#version 440

// ════════════════════════════════════════════════════════════════════════
// brand_banner.frag
//
// One shader, six "motifs" (uMode 0..5), selected per onboarding page from
// C++ (see BannerHost::applyMotifPreset). All motifs share the same base
// gradient, atmosphere layer, and vignette, and the same colorA/colorB/
// accent palette, so switching pages never feels like a different app --
// only the foreground motion differs.
//
// IMPORTANT: the uniform block below must list custom uniforms in EXACTLY
// the order they're declared as properties on each ShaderEffect instance in
// BrandBanner.qml (after the two Qt-supplied members, qt_Matrix/qt_Opacity).
// Qt Quick's shader reflection derives this block's layout from the QML
// declaration order, not from this file. A mismatch compiles fine and
// silently scrambles which QML property drives which uniform.
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
    for (int i = 0; i < 4; i++) {
        v += amp * valueNoise(p);
        p *= 2.02;
        amp *= 0.5;
    }
    return v;
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

// Two overlapping sine waves instead of one -- avoids the metronomic feel
// of a single sin(t), reads as organic "breathing" instead.
float breathe(float t)
{
    return 0.5 + 0.32 * sin(t) + 0.18 * sin(t * 1.9 + 1.3);
}

vec3 baseGradient(vec2 uv)
{
    float d = smoothstep(0.0, 1.0, uv.x * 0.55 + uv.y * 0.45);
    return mix(uColorA.rgb, uColorB.rgb, d);
}

// Slow-drifting soft cloud, shared by every motif underneath the per-mode
// foreground -- keeps the banner from ever reading as a flat procedural
// fill, and ties all six motifs to the same "material".
float atmosphere(vec2 uv, float t)
{
    vec2 p = uv * vec2(2.2, 1.6) + vec2(t * 0.015, -t * 0.01) + uSeed * 3.0;
    return fbm(p);
}

float vignette(vec2 uv)
{
    vec2 c = uv - 0.5;
    c.x *= max(uAspect, 0.0001);
    return smoothstep(0.95, 0.3, length(c));
}

// ── Motifs ─────────────────────────────────────────────────────────────

vec3 motifWelcome(vec2 uv, vec3 base, float t)
{
    vec2 center = vec2(0.46, 0.58);
    float b = breathe(t * 0.6);
    vec3 col = base + uAccent.rgb * bloom(length(uv - center), 0.16, 0.4) * (0.3 + 0.3 * b);

    float motes = 0.0;
    for (int i = 0; i < 5; i++) {
        float fi = float(i);
        vec2 p = uv * vec2(5.0, 2.6) + vec2(uSeed * 8.0 + fi * 4.1, -t * (0.05 + fi * 0.015) - fi * 1.7);
        float mote = smoothstep(0.9, 0.985, valueNoise(p));
        motes += mote * mix(0.25, 0.55, fract(fi * 0.63));
    }
    return col + uAccent.rgb * motes;
}

float dataStreak(vec2 uv, float rowId, float t)
{
    float rowSpeed = 0.35 + hash21(vec2(rowId, 1.0)) * 0.55;
    float phase = hash21(vec2(rowId, 2.0));
    float travel = fract(uv.x - t * rowSpeed * 0.14 + phase);
    float head = smoothstep(0.018, 0.0, travel);
    float tail = exp(-travel * 9.0) * 0.5; // soft comet trail behind the head
    return max(head, tail);
}

vec3 motifCardDatabase(vec2 uv, vec3 base, float t)
{
    vec3 col = base;
    const int ROWS = 7;
    for (int r = 0; r < ROWS; r++) {
        float rowId = float(r) + uSeed * 5.0;
        float rowY = (float(r) + 0.5) / float(ROWS);
        float rowMask = smoothstep(0.045, 0.0, abs(uv.y - rowY));
        float brightness = mix(0.3, 0.8, hash21(vec2(rowId, 3.0)));
        col += uAccent.rgb * dataStreak(uv, rowId, t) * rowMask * brightness * 0.6;
    }
    return col;
}

vec3 motifTheming(vec2 uv, float t)
{
    float wave = sin((uv.x + uv.y) * 3.6 - t * 0.5) * 0.5 + sin((uv.x - uv.y) * 2.1 + t * 0.31) * 0.3;
    wave = wave * 0.5 + 0.5;
    float mixAmt = smoothstep(0.0, 1.0, clamp(uv.x * 0.5 + uv.y * 0.3 + wave * 0.22, 0.0, 1.0));
    vec3 col = mix(uColorA.rgb, uColorB.rgb, mixAmt);
    return col + uAccent.rgb * smoothstep(0.75, 1.0, wave) * 0.3;
}

vec3 motifAccount(vec2 uv, vec3 base, float t)
{
    vec3 col = base;
    const int NODES = 5;
    vec2 pts[NODES];
    for (int i = 0; i < NODES; i++) {
        float fi = float(i);
        pts[i] = vec2(hash21(vec2(fi, uSeed)), hash21(vec2(fi + 10.0, uSeed))) * vec2(0.78, 0.66) +
                 vec2(0.11, 0.17);
    }

    for (int i = 0; i < NODES; i++) {
        vec2 a = pts[i];
        vec2 b = pts[(i + 1) % NODES];
        vec2 pa = uv - a;
        vec2 ba = b - a;
        float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
        float distToLine = length(pa - ba * h);
        col += uAccent.rgb * 0.09 * smoothstep(0.008, 0.0, distToLine);

        float travel = fract(t * 0.16 + float(i) * 0.37 + uSeed);
        float diff = abs(h - travel);
        diff = min(diff, 1.0 - diff); // wrap distance so the pulse loops smoothly, no seam
        col += uAccent.rgb * smoothstep(0.06, 0.0, diff) * smoothstep(0.02, 0.0, distToLine) * 0.7;
    }

    for (int i = 0; i < NODES; i++) {
        float pulse = 0.5 + 0.5 * sin(t * 0.8 + float(i) * 1.7);
        float nodeSize = mix(0.018, 0.03, hash21(vec2(float(i), uSeed + 1.0)));
        col += uAccent.rgb * bloom(length(uv - pts[i]), nodeSize, nodeSize * 3.2) * (0.45 + 0.55 * pulse);
    }
    return col;
}

vec3 motifPreferences(vec2 uv, vec3 base, float t)
{
    vec2 cell = fract(uv * 12.0) - 0.5;
    float gridLine = 1.0 - smoothstep(0.0, 0.05, min(abs(cell.x), abs(cell.y)));
    vec3 col = base + uAccent.rgb * gridLine * 0.045;

    float sweepCoord = fract((uv.x + uv.y) * 0.5 - t * 0.1);
    float sweepDist = min(sweepCoord, 1.0 - sweepCoord);
    col += uAccent.rgb * smoothstep(0.018, 0.0, sweepDist);
    col += uAccent.rgb * smoothstep(0.1, 0.0, sweepDist) * 0.4;
    return col;
}

vec3 motifFinish(vec2 uv, vec3 base, float t)
{
    vec3 col = base;

    float sweepPos = fract(t * 0.07);
    float sweepDist = abs((uv.x * 0.7 + uv.y * 0.3) - sweepPos);
    col += uAccent.rgb * smoothstep(0.02, 0.0, sweepDist);
    col += uAccent.rgb * smoothstep(0.09, 0.0, sweepDist) * 0.4;

    vec2 center = vec2(0.5, 0.52);
    float b = breathe(t * 0.45);
    col += uAccent.rgb * bloom(length(uv - center), 0.32, 0.5) * 0.12 * (0.5 + 0.5 * b);

    float sparkle = 0.0;
    for (int i = 0; i < 3; i++) {
        float fi = float(i);
        vec2 p = uv * vec2(4.0, 2.2) + vec2(uSeed * 6.0 + fi * 5.3, -t * 0.02 - fi * 2.1);
        sparkle += smoothstep(0.94, 0.99, valueNoise(p)) * 0.35;
    }
    return col + uAccent.rgb * sparkle;
}

void main()
{
    vec2 uv = qt_TexCoord0;
    float t = iTime * uSpeed;

    vec3 base = baseGradient(uv);
    base += (atmosphere(uv, iTime) - 0.5) * 0.05;

    vec3 col;
    if (uMode < 0.5) col = motifWelcome(uv, base, t);
    else if (uMode < 1.5) col = motifCardDatabase(uv, base, t);
    else if (uMode < 2.5) col = motifTheming(uv, t);
    else if (uMode < 3.5) col = motifAccount(uv, base, t);
    else if (uMode < 4.5) col = motifPreferences(uv, base, t);
    else col = motifFinish(uv, base, t);

    col *= mix(0.6, 1.0, vignette(uv));
    fragColor = vec4(col, 1.0) * qt_Opacity;
}