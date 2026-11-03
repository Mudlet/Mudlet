#version 330 core
// A tilt-shift photograph's shallow focus: a band across the view stays sharp, and the scene blurs more the further
// above or below it, so the map reads as a small model seen up close
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uScene;
uniform vec2 uTexel;
// The band's middle, from the view's foot to its top
uniform float uFocus;
// How far from the middle the blur starts, and where it is at its strongest, as parts of the view's height
uniform float uSharp;
uniform float uFull;
// The strongest blur's radius, in pixels
uniform float uRadius;

const int TAPS = 16;
const float GOLDEN_ANGLE = 2.39996323;

void main()
{
    vec4 here = texture(uScene, vUv);
    float radius = uRadius * smoothstep(uSharp, uFull, abs(vUv.y - uFocus));
    if (radius < 0.5) {
        FragColor = vec4(here.rgb, 1.0);
        return;
    }
    // Taps spread evenly over a disc along a golden-angle spiral
    vec3 sum = here.rgb;
    for (int tap = 1; tap < TAPS; ++tap) {
        float reach = radius * sqrt(float(tap) / float(TAPS - 1));
        float angle = float(tap) * GOLDEN_ANGLE;
        sum += texture(uScene, vUv + vec2(cos(angle), sin(angle)) * reach * uTexel).rgb;
    }
    FragColor = vec4(sum / float(TAPS), 1.0);
}
