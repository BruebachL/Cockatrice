#version 310 es
precision highp float;

// Uniform block with instance name
layout(std140, binding = 0) uniform TimeBlock {
    float time;
} uTime;

// Sampler with binding
layout(binding = 1) uniform sampler2D textureSource;

// Input/output with explicit locations
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

vec3 hueRotate(vec3 color, float angle) {
    float s = sin(angle);
    float c = cos(angle);

    mat3 rot = mat3(
    vec3(0.299+0.701*c+0.168*s, 0.587-0.587*c+0.330*s, 0.114-0.114*c-0.497*s),
    vec3(0.299-0.299*c-0.328*s, 0.587+0.413*c+0.035*s, 0.114-0.114*c+0.292*s),
    vec3(0.299-0.300*c+1.25*s, 0.587-0.588*c-1.05*s, 0.114+0.886*c-0.203*s)
    );

    return clamp(rot*color, 0.0, 1.0);
}

void main() {
    vec2 uv = qt_TexCoord0;
    vec3 base = texture(textureSource, uv).rgb;

    float t = uTime.time * 0.5;// <-- use instance name here
    float rollX = sin(uv.x * 6.2831 + t);
    float rollY = cos(uv.y * 6.2831 + t);
    float roll = (rollX + rollY) * 0.5;

    float lum = dot(base, vec3(0.299, 0.587, 0.114));
    vec3 rotated = hueRotate(base, roll * 0.5);
    vec3 final = mix(base, rotated, 2.0 * lum);

    fragColor = vec4(final, 1.0);
}
