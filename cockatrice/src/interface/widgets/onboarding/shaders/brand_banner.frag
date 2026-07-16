#version 440

// ════════════════════════════════════════════════════════════════════════
// brand_banner.frag
//
// One shader, six "motifs" (uMode 0..5).  All motifs share a near-black
// backgroundField() with mild ambient flow-noise motion.  Each page gets
// a distinct foreground.  All SDFs operate in aspect-corrected space
// (ac.x = uv.x * uAspect) so shapes look correct on the wide banner.
//
// IMPORTANT: the uniform block below must list custom uniforms in EXACTLY
// the order they're declared as properties on each ShaderEffect instance in
// BrandBanner.qml (after the two Qt-supplied members, qt_Matrix/qt_Opacity).
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

// ── Primitives ──────────────────────────────────────────────────────────

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

float flowNoise(vec2 p, float t)
{
    vec2 warp1 = vec2(fbm(p + vec2(0.0, 0.0)), fbm(p + vec2(5.2, 1.3)));
    vec2 warp2 = vec2(fbm(p + 4.0 * warp1 + vec2(1.7, 9.2) + t * 0.6),
                      fbm(p + 4.0 * warp1 + vec2(8.3, 2.8) - t * 0.5));
    return fbm(p + 4.0 * warp2 + t * 0.15);
}

float bloom(float d, float coreRadius, float haloRadius)
{
    float core = exp(-(d * d) / (coreRadius * coreRadius));
    float halo = exp(-d / haloRadius) * 0.35;
    return core + halo;
}

float roundedBoxSDF(vec2 p, vec2 halfSize, float radius)
{
    vec2 d = abs(p) - halfSize + radius;
    return length(max(d, 0.0)) - radius + min(max(d.x, d.y), 0.0);
}

// Rotated box SDF -- rotates p by angle (radians) before evaluation.
float rotatedBoxSDF(vec2 p, vec2 halfSize, float radius, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    vec2 rp = vec2(p.x * c - p.y * s, p.x * s + p.y * c);
    return roundedBoxSDF(rp, halfSize, radius);
}

float vignette(vec2 uv)
{
    vec2 c = uv - 0.5;
    c.x *= max(uAspect, 0.0001);
    return smoothstep(1.0, 0.25, length(c));
}

// ── Shared background ───────────────────────────────────────────────────

vec3 backgroundField(vec2 uv, float time)
{
    float baseD = smoothstep(0.0, 1.0, uv.y * 0.5 + uv.x * 0.2);
    float painted = flowNoise(uv * 1.5, time * 0.04) - 0.5;
    baseD = clamp(baseD + painted * 0.12, 0.0, 1.0);

    vec3 col = mix(uColorA.rgb, uColorB.rgb, baseD);

    float deep = fbm(uv * 1.0 + vec2(37.1, 12.4) + time * 0.015);
    col = mix(col, uColorB.rgb, (deep - 0.5) * 0.08);

    // Accent fog drifting through the dark
    float fog = flowNoise(uv * 0.8 + vec2(100.0, 50.0), time * 0.02);
    col += uAccent.rgb * max(fog - 0.6, 0.0) * 0.10;

    return col;
}

// ── Motifs ──────────────────────────────────────────────────────────────

// Welcome: subtle ambient glow -- the QML trace is the star here.
vec3 motifWelcome(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;
    float asp = max(uAspect, 0.001);
    vec2 ac = vec2(uv.x * asp, uv.y);
    vec2 center = vec2(asp * 0.5, 0.5);

    // Soft radial accent glow from center -- adds warmth and focus
    float cDist = length(ac - center);
    col += uAccent.rgb * bloom(cDist, 0.25, 0.8) * 0.06;

    // Very faint horizontal light streak
    float streak = exp(-abs(uv.y - 0.5) * 8.0);
    col += uAccent.rgb * streak * 0.02 * (0.8 + 0.2 * sin(t * 0.3));

    return col;
}

// Portrait card silhouettes streaming across the banner at parallax depths.
// Uses hash21 for true pseudo-random placement; slightly slower drift.
vec3 motifCardDatabase(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;
    float asp = max(uAspect, 0.001);
    vec2 ac = vec2(uv.x * asp, uv.y);

    const int CARDS = 25;
    for (int i = 0; i < CARDS; i++) {
        float fi = float(i);

        // True pseudo-random depth via hash
        float depth = hash21(vec2(fi * 1.37 + uSeed, fi * 0.91));

        // Card half-sizes in corrected space (portrait: hh > hw)
        float cardH = mix(0.055, 0.15, depth);
        // Random size jitter: +/- 15%
        cardH *= 0.85 + 0.30 * hash21(vec2(fi * 3.14, uSeed * 2.71));
        float cardW = cardH * 0.71; // 5 : 7 playing-card ratio

        // Slower horizontal drift -- near cards faster
        float speed = mix(0.06, 0.18, depth);
        float xPhase = hash21(vec2(fi * 7.13, uSeed * 4.37));
        xPhase = fract(xPhase + t * speed);
        float x = mix(-1.5, asp + 1.5, xPhase);

        // Vertical: hash-distributed with gentle sinusoidal bob
        float yBase = hash21(vec2(fi * 2.91, uSeed * 1.63));
        float y = yBase + sin(t * 0.6 + fi * 1.9) * 0.035;
        y = clamp(y, cardH + 0.02, 1.0 - cardH - 0.02);

        // Small random rotation for organic feel (±4 degrees)
        float tilt = (hash21(vec2(fi * 5.71, uSeed * 8.29)) - 0.5) * 0.14;

        // SDF in corrected space with rotation
        vec2 p = ac - vec2(x, y);
        float d = rotatedBoxSDF(p, vec2(cardW, cardH), cardW * 0.14, tilt);

        // Semi-transparent dark fill
        float fill = smoothstep(0.015, -0.005, d);
        col = mix(col, uColorB.rgb * 0.55, fill * 0.50);

        // Bright accent outline
        float edge = smoothstep(0.035, 0.0, abs(d));
        col += uAccent.rgb * edge * mix(0.18, 0.50, 1.0 - depth);

        // Card-back pattern: inner diamond / cross motif
        float innerD = rotatedBoxSDF(p, vec2(cardW * 0.45, cardH * 0.55), cardW * 0.08, tilt);
        float innerEdge = smoothstep(0.012, 0.0, abs(innerD));
        col += uAccent.rgb * innerEdge * fill * 0.12 * (1.0 - depth);

        // Center dot on the card
        float dotDist = length(p);
        col += uAccent.rgb * bloom(dotDist, 0.008, 0.02) * fill * 0.15 * (1.0 - depth);
    }
    return col;
}

// Flowing aurora colour bands -- accent green is the dominant bright colour.
vec3 motifTheming(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;

    const int BANDS = 4;
    for (int i = 0; i < BANDS; i++) {
        float fi = float(i);
        float yCenter = 0.2 + fi * 0.2;

        // Multi-frequency undulation for organic motion
        float wave = sin(uv.x * 3.5 + t * 0.6 + fi * 2.1) * 0.09;
        wave += sin(uv.x * 8.0 - t * 0.4 + fi * 1.3) * 0.03;
        wave += sin(uv.x * 1.5 + t * 0.25 + fi * 3.7) * 0.05;

        float bandDist = abs(uv.y - yCenter - wave);
        float bandWidth = 0.055 + sin(t * 0.25 + fi * 0.9) * 0.015;
        float band = smoothstep(bandWidth, 0.0, bandDist);

        float intensity = mix(0.18, 0.40, 1.0 - fi / float(BANDS));
        col += uAccent.rgb * band * intensity;
    }

    return col;
}

// Distributed network graph filling the full banner -- connection theme.
// Nodes spread across the space with connections between nearby ones,
// pulsing ripples, and a central hub glow.
vec3 motifAccount(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;
    float asp = max(uAspect, 0.001);
    vec2 ac = vec2(uv.x * asp, uv.y);
    vec2 center = vec2(asp * 0.5, 0.5);

    // 14 nodes distributed across the full banner via hash
    const int NODES = 14;
    vec2 nodePos[14];
    float nodePulse[14];

    for (int i = 0; i < NODES; i++) {
        float fi = float(i);

        // Pseudo-random position spread across the banner
        float nx = hash21(vec2(fi * 3.17 + uSeed, fi * 1.93)) * asp;
        float ny = hash21(vec2(fi * 5.41 + uSeed * 1.7, fi * 2.79));

        // Gentle drift
        float dx = sin(t * 0.12 + fi * 1.7) * 0.08;
        float dy = cos(t * 0.09 + fi * 2.3) * 0.04;
        vec2 pos = vec2(nx + dx, ny + dy);
        nodePos[i] = pos;

        // Per-node pulse phase
        float pulsePhase = hash21(vec2(fi * 4.31, uSeed * 6.17));
        float pulse = sin(t * 0.8 + pulsePhase * 6.283) * 0.5 + 0.5;
        nodePulse[i] = pulse;

        // Node glow -- fills space with distributed light
        float dist = length(ac - pos);
        col += uAccent.rgb * bloom(dist, 0.018, 0.08) * mix(0.20, 0.45, pulse);
    }

    // Connection lines between nodes within a distance threshold
    float connectDist = asp * 0.22; // connection radius in corrected space
    for (int i = 0; i < NODES; i++) {
        for (int j = i + 1; j < NODES; j++) {
            float pairDist = length(nodePos[i] - nodePos[j]);
            if (pairDist < connectDist) {
                float strength = 1.0 - pairDist / connectDist;
                vec2 pa = ac - nodePos[i];
                vec2 ba = nodePos[j] - nodePos[i];
                float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
                float lineDist = length(pa - ba * h);
                col += uAccent.rgb * smoothstep(0.010, 0.0, lineDist) * strength * 0.10;
            }
        }
    }

    // Central hub glow -- steady, warm
    float cDist = length(ac - center);
    col += uAccent.rgb * bloom(cDist, 0.04, 0.25) * 0.12;

    // Periodic ripple from center
    float ripplePhase = t * 0.4;
    float rippleDist = abs(cDist - fract(ripplePhase) * asp * 0.7);
    col += uAccent.rgb * smoothstep(0.02, 0.0, rippleDist) * 0.10;

    return col;
}

// Toggle-grid with a scanning highlight -- settings / preferences theme.
vec3 motifPreferences(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;

    // Grid: 18 columns x 5 rows of small indicator squares
    float cols = 18.0;
    float rows = 5.0;
    vec2 gridUV = uv * vec2(cols, rows);
    vec2 cell = fract(gridUV) - 0.5;
    vec2 cellId = floor(gridUV);

    // "On" state per cell
    float on = step(0.55, hash21(cellId + uSeed * 10.0));

    float d = roundedBoxSDF(cell, vec2(0.28, 0.32), 0.06);

    // Filled "on" cells glow with accent
    float cellFill = smoothstep(0.04, -0.02, d);
    col += uAccent.rgb * cellFill * on * 0.18;

    // Thin cell border
    float border = smoothstep(0.025, 0.0, abs(d));
    col += uAccent.rgb * border * 0.06;

    // Bright scanning highlight sweeping left to right
    float scanX = fract(t * 0.15);
    float scanDist = abs(uv.x - scanX);
    float scanLine = smoothstep(0.015, 0.0, scanDist);
    col += uAccent.rgb * scanLine * 0.40;

    // Wider soft glow around the scan line
    col += uAccent.rgb * smoothstep(0.08, 0.0, scanDist) * 0.08;

    // "Lit" cells near the scan line glow brighter
    float scanProximity = smoothstep(0.12, 0.0, scanDist);
    col += uAccent.rgb * cellFill * on * scanProximity * 0.15;

    return col;
}

// Expanding concentric rings + radial glow + drifting sparkles.
// Fills the full banner with a satisfying "completion" wash.
vec3 motifFinish(vec2 uv, vec3 bg, float t)
{
    vec3 col = bg;
    float asp = max(uAspect, 0.001);
    vec2 ac = vec2(uv.x * asp, uv.y);
    vec2 center = vec2(asp * 0.5, 0.5);
    float cDist = length(ac - center);

    // Expanding concentric accent rings -- each at a different phase
    const int RINGS = 5;
    for (int i = 0; i < RINGS; i++) {
        float fi = float(i);
        float phase = fract(t * 0.08 + fi * 0.2);
        float ringRadius = phase * asp * 0.65;
        float ringDist = abs(cDist - ringRadius);
        float ringWidth = 0.015 + phase * 0.01; // wider as it expands
        float ring = smoothstep(ringWidth, 0.0, ringDist);
        float fade = 1.0 - phase * 0.6; // fade as it expands
        col += uAccent.rgb * ring * fade * 0.20;
    }

    // Steady radial glow from center
    col += uAccent.rgb * bloom(cDist, 0.06, 0.35) * 0.10;

    // Drifting sparkles -- larger, fewer, more visible than confetti
    const int SPARKLES = 12;
    for (int i = 0; i < SPARKLES; i++) {
        float fi = float(i);
        float px = hash21(vec2(fi * 127.1, uSeed * 311.7));
        float py = hash21(vec2(fi * 269.5, uSeed * 183.3));
        float pSpeed = 0.03 + fract(fi * 0.317) * 0.05;
        float pY = fract(py + t * pSpeed);
        float pX = px + sin(t * 0.25 + fi * 2.1) * 0.08;
        vec2 pPos = vec2(pX * asp, pY);
        float pDist = length(ac - pPos);
        // Twinkle
        float twinkle = sin(t * 1.5 + fi * 3.7) * 0.5 + 0.5;
        col += uAccent.rgb * bloom(pDist, 0.010, 0.04) * mix(0.15, 0.35, twinkle);
    }

    return col;
}

// ── Main ────────────────────────────────────────────────────────────────

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
