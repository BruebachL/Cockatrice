#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    float iTime;
    vec2 iResolution;
};

const float s3 = 1.7320508075688772;
const float i3 = 0.5773502691896258;

const mat2 tri2cart = mat2(
1.0, 0.0,
-0.5, 0.5 * s3
);

const mat2 cart2tri = mat2(
1.0, 0.0,
i3, 2.0 * i3
);

vec3 pal(float t)
{
    vec3 a = vec3(0.5);
    vec3 b = vec3(0.5);
    vec3 c = vec3(0.8, 0.8, 0.5);
    vec3 d = vec3(0.0, 0.2, 0.5);

    return clamp(
    a + b * cos(6.28318 * (c * t + d)),
    0.0,
    1.0
    );
}

#define HASHSCALE1 0.1031
const vec3 HASHSCALE3 = vec3(443.897, 441.423, 437.195);

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash23(vec3 p3)
{
    p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx + p3.yz) * p3.zy);
}

vec3 bary(vec2 v0, vec2 v1, vec2 v2)
{
    float inv_denom =
    1.0 / (v0.x * v1.y - v1.x * v0.y);

    float v =
    (v2.x * v1.y - v1.x * v2.y) * inv_denom;

    float w =
    (v0.x * v2.y - v2.x * v0.y) * inv_denom;

    float u = 1.0 - v - w;

    return vec3(u, v, w);
}

float dseg(vec2 xa, vec2 ba)
{
    return length(
    xa - ba * clamp(
    dot(xa, ba) / dot(ba, ba),
    0.0,
    1.0
    )
    );
}

vec2 randCircle(vec3 p)
{
    vec2 rt = hash23(p);

    float r = sqrt(rt.x);
    float theta = 6.28318530718 * rt.y;

    return r * vec2(cos(theta), sin(theta));
}

vec2 randCircleSpline(vec2 p, float t)
{
    float t1 = floor(t);
    t -= t1;

    vec2 pa = randCircle(vec3(p, t1 - 1.0));
    vec2 p0 = randCircle(vec3(p, t1));
    vec2 p1 = randCircle(vec3(p, t1 + 1.0));
    vec2 pb = randCircle(vec3(p, t1 + 2.0));

    vec2 m0 = 0.5 * (p1 - pa);
    vec2 m1 = 0.5 * (pb - p0);

    vec2 c3 = 2.0 * p0 - 2.0 * p1 + m0 + m1;
    vec2 c2 = -3.0 * p0 + 3.0 * p1 - 2.0 * m0 - m1;
    vec2 c1 = m0;
    vec2 c0 = p0;

    return (((c3 * t + c2) * t + c1) * t + c0) * 0.8;
}

vec2 triPoint(vec2 p)
{
    float t0 = hash12(p);

    return tri2cart * p
    + 0.45 * randCircleSpline(
    p,
    0.15 * iTime + t0
    );
}

void tri_color(
vec2 p,
vec4 t0,
vec4 t1,
vec4 t2,
float scl,
inout vec4 cw
)
{
    vec2 p0 = p - t0.xy;
    vec2 p10 = t1.xy - t0.xy;
    vec2 p20 = t2.xy - t0.xy;

    vec3 b = bary(p10, p20, p0);

    float d10 = dseg(p0, p10);
    float d20 = dseg(p0, p20);
    float d21 = dseg(
    p - t1.xy,
    t2.xy - t1.xy
    );

    float d = min(min(d10, d20), d21);

    d *= -sign(min(b.x, min(b.y, b.z)));

    if (d < 0.5 * scl)
    {
        vec2 tsum =
        t0.zw + t1.zw + t2.zw;

        vec3 h_tri = vec3(
        hash12(tsum + t0.zw),
        hash12(tsum + t1.zw),
        hash12(tsum + t2.zw)
        );

        vec2 pctr =
        (t0.xy + t1.xy + t2.xy) / 3.0;

        float theta =
        1.0 + 0.01 * iTime;

        vec2 dir =
        vec2(cos(theta), sin(theta));

        float grad_input =
        dot(pctr, dir)
        - sin(0.05 * iTime);

        float h0 =
        sin(0.7 * grad_input)
        * 0.5 + 0.5;

        h_tri = mix(vec3(h0), h_tri, 0.4);

        float h = dot(h_tri, b);

        vec3 c = pal(h);

        float w = smoothstep(
        0.5 * scl,
        -0.5 * scl,
        d
        );

        cw += vec4(w * c, w);
    }
}

void main()
{
    vec2 fragCoord =
    qt_TexCoord0 * iResolution;

    float scl = 4.1 / iResolution.y;

    vec2 p =
    (fragCoord
    - 0.5
    - 0.5 * iResolution.xy) * scl;

    vec2 tfloor =
    floor(cart2tri * p + 0.5);

    vec2 pts[9];

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            pts[3 * i + j] =
            triPoint(
            tfloor
            + vec2(
            float(i - 1),
            float(j - 1)
            )
            );
        }
    }

    vec4 cw = vec4(0.0);

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            vec4 t00 = vec4(
            pts[3 * i + j],
            tfloor + vec2(
            float(i - 1),
            float(j - 1)
            )
            );

            vec4 t10 = vec4(
            pts[3 * i + j + 3],
            tfloor + vec2(
            float(i),
            float(j - 1)
            )
            );

            vec4 t01 = vec4(
            pts[3 * i + j + 1],
            tfloor + vec2(
            float(i - 1),
            float(j)
            )
            );

            vec4 t11 = vec4(
            pts[3 * i + j + 4],
            tfloor + vec2(
            float(i),
            float(j)
            )
            );

            tri_color(
            p,
            t00,
            t10,
            t11,
            scl,
            cw
            );

            tri_color(
            p,
            t00,
            t11,
            t01,
            scl,
            cw
            );
        }
    }

    fragColor = vec4(cw.rgb / max(cw.w, 0.0001), 1.0)
    * qt_Opacity;
}