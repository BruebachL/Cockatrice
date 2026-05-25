#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    float iTime;
    vec2 iResolution;
};

//////////////////////////////////////////////////////

const vec3 bgColor   = vec3(0.01, 0.16, 0.42);
const vec3 rectColor = vec3(0.01, 0.26, 0.57);

const float noiseIntensity = 2.8;
const float noiseDefinition = 0.6;
const vec2 glowPos = vec2(-2.0, 0.0);

const float total = 0.0;
const float minSize = 0.03;
const float maxSize = 0.08 - minSize;
const float yDistribution = 0.5;

//////////////////////////////////////////////////////

float random(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float noise(vec2 p)
{
    p *= noiseIntensity;

    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(
    mix(random(i + vec2(0.0, 0.0)),
    random(i + vec2(1.0, 0.0)), u.x),
    mix(random(i + vec2(0.0, 1.0)),
    random(i + vec2(1.0, 1.0)), u.x),
    u.y
    );
}

float fbm(vec2 uv)
{
    uv *= 5.0;

    mat2 m = mat2(
    1.6, 1.2,
    -1.2, 1.6
    );

    float f = 0.5 * noise(uv); uv = m * uv;
    f += 0.25 * noise(uv); uv = m * uv;
    f += 0.125 * noise(uv); uv = m * uv;
    f += 0.0625 * noise(uv);

    return 0.5 + 0.5 * f;
}

//////////////////////////////////////////////////////

vec3 bg(vec2 uv)
{
    float velocity = iTime / 1.6;

    float intensity =
    sin(uv.x * 3.0 + velocity * 2.0) * 1.1 + 1.5;

    uv.y -= 2.0;

    vec2 bp = uv + glowPos;

    uv *= noiseDefinition;

    float rb = fbm(vec2(uv.x * 0.5 - velocity * 0.03, uv.y)) * 0.1;
    uv += rb;

    float rz = fbm(uv * 0.9 + vec2(velocity * 0.35, 0.0));
    rz *= dot(bp * intensity, bp) + 1.2;

    vec3 col = bgColor / (0.1 - rz);

    return sqrt(abs(col));
}

//////////////////////////////////////////////////////

float rectangle(vec2 uv, vec2 pos, float w, float h, float blur)
{
    vec2 d = (vec2(w, h) + 0.01) / 2.0 - abs(uv - pos);
    d = smoothstep(0.0, blur, d);
    return d.x * d.y;
}

mat2 rotate2d(float a)
{
    float s = sin(a);
    float c = cos(a);
    return mat2(c, -s, s, c);
}

//////////////////////////////////////////////////////

void main()
{
    vec2 uv = qt_TexCoord0 * iResolution;
    uv = uv / iResolution * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    vec3 color = bg(uv) * (2.0 - abs(uv.y * 2.0));

    float velX = -iTime / 16.0;
    float velY =  iTime / 20.0;

    for (float i = 0.0; i < 1.0; i += 1.0)
    {
        float index = i;
        float rnd = random(vec2(index));

        vec3 pos = vec3(0.0);

        pos.x = fract(velX * rnd + index) * 4.0 - 2.0;
        pos.y = sin(index * rnd * 1000.0 + velY) * yDistribution;
        pos.z = maxSize * rnd + minSize;

        vec2 uvRot = uv - pos.xy + pos.z / 2.0;
        uvRot = rotate2d(i + iTime / 2.0) * uvRot;
        uvRot += pos.xy + pos.z / 2.0;

        float rect = rectangle(
        uvRot,
        pos.xy,
        pos.z,
        pos.z,
        (maxSize + minSize - pos.z) / 2.0
        );

        color += rectColor * rect * pos.z / maxSize;
    }

    fragColor = vec4(color, 1.0) * qt_Opacity;
}