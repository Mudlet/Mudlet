#version 330 core
in vec2 uv;
out vec4 FragColor;

uniform sampler2D uSource;
// 0: what is brighter than uThreshold, averaged over the 4x4 source texels under each target texel
// 1: blurred along uDirection, in source texels
// 2: scaled by uStrength, to be added onto the frame
uniform int uPass = 0;
uniform vec2 uTexel;
uniform vec2 uDirection;
uniform float uThreshold = 0.45;
uniform float uStrength = 1.0;

vec3 bright(vec2 at)
{
    vec3 color = texture(uSource, at).rgb;
    return color * smoothstep(uThreshold, uThreshold + 0.3, max(color.r, max(color.g, color.b)));
}

void main()
{
    if (uPass == 0) {
        // Each tap sits between four texels, which linear filtering averages
        vec3 sum = bright(uv + vec2(-uTexel.x, -uTexel.y)) + bright(uv + vec2(uTexel.x, -uTexel.y))
                 + bright(uv + vec2(-uTexel.x, uTexel.y)) + bright(uv + vec2(uTexel.x, uTexel.y));
        FragColor = vec4(sum * 0.25, 1.0);
    } else if (uPass == 1) {
        // A nine texel Gaussian in five taps, the outer ones between pairs of texels
        vec2 step1 = uDirection * uTexel * 1.3846153846;
        vec2 step2 = uDirection * uTexel * 3.2307692308;
        vec3 sum = texture(uSource, uv).rgb * 0.2270270270;
        sum += (texture(uSource, uv + step1).rgb + texture(uSource, uv - step1).rgb) * 0.3162162162;
        sum += (texture(uSource, uv + step2).rgb + texture(uSource, uv - step2).rgb) * 0.0702702703;
        FragColor = vec4(sum, 1.0);
    } else {
        FragColor = vec4(texture(uSource, uv).rgb * uStrength, 1.0);
    }
}
