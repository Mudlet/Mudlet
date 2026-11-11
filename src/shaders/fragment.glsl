#version 330 core
in vec4 vertexColor;
in vec2 texCoord;
in float eyeDepth;
in vec3 localPosition;
in vec3 localNormal;
out vec4 FragColor;

uniform bool uUseTexture = false;
uniform bool uUsePBR = false;
uniform sampler2D uTexture;           // Legacy single texture or base color
uniform sampler2D uBaseColorTexture;  // PBR base color
uniform sampler2D uMetallicRoughnessTexture; // PBR metallic/roughness
uniform sampler2D uNormalTexture;     // PBR normal map

// PBR material factors
uniform vec4 uBaseColorFactor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;

// Toward the background with distance, from no fog at uFogStart to uFogMax at uFogEnd; none when uFogEnd <= uFogStart
uniform vec3 uFogColor = vec3(0.0, 0.0, 0.0);
uniform float uFogStart = 0.0;
uniform float uFogEnd = 0.0;
uniform float uFogMax = 0.0;

// Draws only the edges of the [-1,1] cube or cylinder template, as a line uWireWidth pixels wide with a glow falling off
// over uWireGlow pixels and a faint uWireFill across the faces
uniform bool uWireframe = false;
uniform bool uWireRound = false;
uniform float uWireSides = 24.0;
uniform float uWireWidth = 1.5;
uniform float uWireGlow = 5.0;
uniform float uWireFill;
// Darkens every uScanlinePeriod-th row of pixels by uScanlines
uniform float uScanlines = 0.0;
uniform float uScanlinePeriod = 3.0;
// Inkwash: 1 a blot, 2 a stroke, 3 a seal, 4 the paper, 5 a lozenge, 6 a hatched one, 7 a dashed one. Uneven by where
// an instance stands or by the pixel, never by time, so nothing shimmers
uniform int uInk = 0;
uniform bool uInkRound = false;
flat in vec3 instanceOrigin;
// Diorama: 1 a block, 2 a path, 3 a shadow on the ground, 4 the sky, 5 water. Lit by a soft light of its own from high
// in the north-west, which a block's rounded top edges catch on that side and lose on the other; a cone, a roof or a
// ball has no such edges
uniform int uModelShading = 0;
uniform bool uModelRound = false;
uniform bool uModelBevel = true;
// A see-through model's edges, drawn solid so that it reads as a building rather than a smudge: 0 for none, 1 for the
// cube, 2 the cylinder, 3 the pyramid and 4 the cone template's
uniform int uModelEdges = 0;
in vec3 surfaceNormal;
flat in vec3 instanceSide;
const vec3 cModelLight = vec3(-0.45, 0.55, 0.7);

// pcg3d, a hash that gives the same on every GPU
uint inkHash(uvec3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v.x ^ v.y ^ v.z;
}

float inkRandom(ivec2 cell, uint seed)
{
    return float(inkHash(uvec3(uvec2(cell), seed)) & 0xffffu) / 65535.0;
}

// Smooth noise from 0 to 1, changing over about a unit of p
float inkNoise(vec2 p, uint seed)
{
    vec2 cell = floor(p);
    vec2 f = p - cell;
    f = f * f * (3.0 - 2.0 * f);
    ivec2 i = ivec2(cell);
    return mix(mix(inkRandom(i, seed), inkRandom(i + ivec2(1, 0), seed), f.x), mix(inkRandom(i + ivec2(0, 1), seed), inkRandom(i + ivec2(1, 1), seed), f.x), f.y);
}

uint inkSeed()
{
    // A room stands on whole map units, raised a little while hovered; a bar's middle can fall between them
    return inkHash(uvec3(ivec3(ivec2(round(instanceOrigin.xy * 4.0)), int(round(instanceOrigin.z)))));
}

// The paper's own unevenness, a little under 1, by the pixel
float paperGrain()
{
    return 0.94 + 0.06 * inkRandom(ivec2(gl_FragCoord.xy), 7u);
}

// How far this point of the [-1,1] template is from its nearest edge, in the template's units: 1 for the cube, 2 the
// cylinder, 3 the pyramid and 4 the cone
float templateEdge(int shape)
{
    if (shape == 1) {
        // A face's own axis is pushed out of reach so only the two axes along the face make edges
        vec3 inward = 1.0 - abs(localPosition) + step(0.5, abs(localNormal)) * 2.0;
        return min(min(inward.x, inward.y), inward.z);
    }
    bool base = localNormal.z < -0.5;
    if (shape == 3) {
        vec2 a = abs(localPosition.xy);
        return base ? 1.0 - max(a.x, a.y) : min(localPosition.z + 1.0, abs(a.x - a.y));
    }
    if (shape == 2 ? abs(localNormal.z) > 0.5 : base) {
        // To the cap's nearest side: a circle's radius would leave the middle of each side short of the rim, zoomed in
        float side = 6.2831853 / uWireSides;
        float middle = (floor(atan(localPosition.y, localPosition.x) / side) + 0.5) * side;
        return 1.0 - dot(localPosition.xy, vec2(cos(middle), sin(middle))) / cos(side * 0.5);
    }
    return shape == 2 ? 1.0 - abs(localPosition.z) : localPosition.z + 1.0;
}

void main()
{
    if (uUsePBR) {
            vec4 baseColor = texture(uBaseColorTexture, texCoord);
            // Convert base color from sRGB to linear (approximation)
            baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
            // Apply base color factor from material
            baseColor *= uBaseColorFactor;
            
            vec3 metallicRoughness = texture(uMetallicRoughnessTexture, texCoord).rgb;
            // Metallic/roughness should already be linear - no conversion needed
            
            vec3 normalMap = texture(uNormalTexture, texCoord).rgb;
            // Unpack normal map from [0,1] to [-1,1] range
            vec3 normal = normalize(normalMap * 2.0 - 1.0);
            
            // Use proper glTF channel mapping with material factors
            // Standard glTF: B=metallic, G=roughness
            float metallic = metallicRoughness.b * uMetallicFactor;
            float roughness = metallicRoughness.g * uRoughnessFactor;
            
            // Proper PBR lighting setup
            vec3 lightDir = normalize(vec3(0.5, 0.7, 1.0)); // Light direction
            vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0));   // Camera direction
            vec3 halfVector = normalize(lightDir + viewDir);   // Halfway vector for specular
            vec3 lightColor = vec3(1.0, 1.0, 1.0);
            
            // Dot products for lighting
            float NdotL = max(dot(normal, lightDir), 0.0);
            float NdotV = max(dot(normal, viewDir), 0.0);
            float NdotH = max(dot(normal, halfVector), 0.0);
            
            // F0 (base reflectivity) - varies between dielectric (0.04) and metallic (baseColor)
            vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);
            
            // Simple Fresnel approximation
            vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
            
            // Distribution term - controls specular highlight size based on roughness
            float alpha = roughness * roughness;
            float alpha2 = alpha * alpha;
            float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
            float distribution = alpha2 / (3.14159 * denom * denom);
            
            // Simple geometry term
            float geometry = min(1.0, min(2.0 * NdotH * NdotV / max(dot(viewDir, halfVector), 0.001), 
                                         2.0 * NdotH * NdotL / max(dot(viewDir, halfVector), 0.001)));
            
            // Cook-Torrance specular
            vec3 specular = (distribution * geometry * fresnel) / max(4.0 * NdotV * NdotL, 0.001);
            
            // Energy conserving diffuse (metals have no diffuse)
            vec3 kS = fresnel;  // Specular contribution
            vec3 kD = vec3(1.0) - kS;  // Diffuse contribution
            kD *= (1.0 - metallic);  // Metals have no diffuse
            
            // Lambert diffuse
            vec3 diffuse = kD * baseColor.rgb / 3.14159;
            
            // Combine lighting
            vec3 color = (diffuse + specular) * lightColor * NdotL;
            
            // Add ambient - increased for rough metallic materials
            vec3 ambient = vec3(0.15) * baseColor.rgb;
            vec3 finalColor = ambient + color;
            
            // Apply vertex color
            finalColor = finalColor * vertexColor.rgb;
            
            // Convert back from linear to sRGB for display
            finalColor = pow(finalColor, vec3(1.0/2.2));
            
            FragColor = vec4(finalColor, baseColor.a * vertexColor.a);
    } else if (uUseTexture) {
        // Simple texture rendering
        vec4 textureColor = texture(uTexture, texCoord);
        FragColor = textureColor * vertexColor;
    } else if (uWireframe) {
        float edge = templateEdge(uWireRound ? 2 : 1);
        float pixels = edge / max(fwidth(edge), 1e-6);
        float line = clamp(uWireWidth + 0.5 - pixels, 0.0, 1.0);
        float glow = exp(-pixels / uWireGlow);
        float light = min(max(line, 0.6 * glow) + uWireFill, 1.0);
        FragColor = vec4(mix(vertexColor.rgb, vec3(1.0), 0.45 * line), vertexColor.a * light);
    } else if (uInk == 1 || uInk == 3) {
        bool seal = uInk == 3;
        uint seed = inkSeed();
        vec2 p = localPosition.xy;
        // How far inside the shape, in halves of the room's width: a square's corners rounded as a brush leaves them
        float inside;
        if (uInkRound) {
            inside = 1.0 - length(p);
        } else {
            float corner = seal ? 0.08 : 0.35;
            vec2 q = abs(p) - vec2(1.0 - corner);
            inside = corner - length(max(q, 0.0)) - min(max(q.x, q.y), 0.0);
        }
        // The rim wanders in and out, slowly round the blot and finely as the brush's hairs leave it, never past the
        // shape, which is all there is to draw on
        float wander = seal ? 0.03 : 0.3;
        vec2 around = p / max(length(p), 1e-4);
        float rim = (inkNoise(around * 2.2 + 4.0, seed) - 0.5) * wander + (inkNoise(around * 7.0 + 9.0, seed + 1u) - 0.5) * wander * 0.5;
        float edge = inside - 0.5 * wander - 0.04 - rim;
        // Ink thinned for another level spreads further into the paper
        float feather = max(mix(0.35, seal ? 0.04 : 0.1, vertexColor.a), 1.5 * fwidth(inside));
        float cover = smoothstep(0.0, feather, edge);
        vec3 color = vertexColor.rgb;
        float alpha;
        if (seal) {
            // A frame cut into the stamp, where the paper shows, and the paste pressed on unevenly
            float cut = smoothstep(0.12, 0.15, edge) * (1.0 - smoothstep(0.22, 0.25, edge));
            alpha = cover * (1.0 - 0.85 * cut) * (0.85 + 0.15 * inkNoise(p * 7.0, seed + 3u));
        } else {
            // Ink pools darker at the edge of a wash, and lies unevenly inside it
            float pool = 1.0 - smoothstep(0.0, 0.3, edge);
            color *= 1.0 - 0.3 * pool;
            alpha = cover * mix(0.8 + 0.2 * inkNoise(p * 2.5 + 2.0, seed + 2u), 1.0, pool);
        }
        alpha *= paperGrain() * vertexColor.a;
        // Brushed onto the paper, a room has only its top; left until after fwidth(), which a discard before it would upset
        if (localNormal.z < 0.5 || alpha < 0.004) {
            discard;
        }
        FragColor = vec4(color, alpha);
    } else if (uInk == 2) {
        uint seed = inkSeed();
        // From 0 where the stroke starts to 1 where the brush lifts
        float along = localPosition.x * 0.5 + 0.5;
        // Across the stroke on whichever face shows: a bar standing upright, as an up or down exit's, turns its top
        // to the side and shows only its end from above
        float across = abs(localNormal.z) > 0.5 ? localPosition.y : (abs(localNormal.y) > 0.5 ? localPosition.z : max(abs(localPosition.y), abs(localPosition.z)));
        float reach = 0.9 - 0.55 * smoothstep(0.1, 0.95, along) + (inkNoise(vec2(along * 5.0, 0.5), seed) - 0.5) * 0.2;
        float feather = max(0.12, 1.5 * fwidth(across));
        float cover = smoothstep(0.0, feather, reach - abs(across));
        // Running dry toward its end, the brush leaves streaks along the stroke
        float streaks = smoothstep(0.2, 0.55, inkNoise(vec2(along * 2.0, across * 4.0), seed + 1u));
        float alpha = cover * mix(1.0, streaks, 0.7 * along * along) * paperGrain() * vertexColor.a;
        if (alpha < 0.004) {
            discard;
        }
        FragColor = vec4(vertexColor.rgb, alpha);
    } else if (uInk == 4) {
        vec2 at = gl_FragCoord.xy;
        float cloud = inkNoise(at / 120.0, 12u);
        // Rice paper's long fibres, laid two slanting ways so that they cross as no grid would
        vec2 one = mat2(0.8, 0.6, -0.6, 0.8) * at;
        vec2 two = mat2(0.5, -0.87, 0.87, 0.5) * at;
        float fibres = max(inkNoise(vec2(one.x / 2.5, one.y / 55.0), 13u), inkNoise(vec2(two.x / 2.5, two.y / 70.0), 14u));
        float shade = paperGrain() + 0.025 - 0.05 * cloud - 0.03 * smoothstep(0.85, 0.98, fibres);
        FragColor = vec4(vertexColor.rgb * shade, vertexColor.a);
    } else if (uModelShading == 1 || uModelShading == 2) {
        vec3 normal = normalize(surfaceNormal);
        // A path standing upright, as an up or down exit's, turns its template's top to the side
        bool top = uModelShading == 1 ? localNormal.z > 0.5 : normal.z > 0.5;
        if (uModelShading == 1 && uModelBevel) {
            if (top) {
                // Leaning outward over the last part of the way to the rim, as a rounded edge does
                vec2 outward;
                float inside;
                if (uModelRound) {
                    outward = localPosition.xy / max(length(localPosition.xy), 1e-4);
                    inside = 1.0 - length(localPosition.xy);
                } else {
                    vec2 a = abs(localPosition.xy);
                    outward = a.x > a.y ? vec2(sign(localPosition.x), 0.0) : vec2(0.0, sign(localPosition.y));
                    inside = 1.0 - max(a.x, a.y);
                }
                float lean = 1.0 - smoothstep(0.0, 0.16, inside);
                normal = normalize(vec3(outward * 1.3 * lean, 1.0));
            } else if (abs(localNormal.z) < 0.5) {
                // The same edge seen from the side, curling up into the top
                normal = normalize(normal + vec3(0.0, 0.0, 1.3 * smoothstep(0.7, 1.0, localPosition.z)));
            }
        } else if (uModelShading == 2 && top) {
            // A path's crown, highest along its middle
            normal = normalize(normal + instanceSide * localPosition.y * 0.6);
        }
        float diffuse = max(dot(normal, normalize(cModelLight)), 0.0);
        float light = 0.42 + 0.22 * (0.5 + 0.5 * normal.z) + 0.45 * diffuse;
        // Darker toward the ground, where little light reaches between the blocks
        if (uModelShading == 1 && uModelBevel && !top && localNormal.z > -0.5) {
            light *= mix(0.6, 1.0, smoothstep(-1.0, 0.6, localPosition.z));
        }
        vec3 shaded = vertexColor.rgb * light * vec3(1.03, 1.0, 0.95);
        float alpha = vertexColor.a;
        if (uModelShading == 1 && uModelEdges > 0 && alpha < 0.999) {
            float edge = templateEdge(uModelEdges);
            float line = clamp(uWireWidth + 0.5 - edge / max(fwidth(edge), 1e-6), 0.0, 1.0);
            shaded *= 1.0 - 0.5 * line * min(1.0, (1.0 - alpha) / 0.7);
            alpha = mix(alpha, 1.0, line);
        }
        FragColor = vec4(shaded, alpha);
    } else if (uModelShading == 3) {
        // A soft blob on the ground, rounded at a square's corners, fading out by its rim
        if (localNormal.z < 0.5) {
            discard;
        }
        vec2 a = abs(localPosition.xy);
        float reach = uModelRound ? length(a) : min(max(a.x, a.y), 0.45) + length(max(a - 0.45, 0.0));
        float alpha = (1.0 - smoothstep(0.45, 1.0, reach)) * vertexColor.a;
        if (alpha < 0.004) {
            discard;
        }
        FragColor = vec4(vertexColor.rgb, alpha);
    } else if (uModelShading == 4) {
        // A gradient this gentle bands on an 8-bit screen unless each pixel is nudged by less than a step
        float dither = (inkRandom(ivec2(gl_FragCoord.xy), 21u) - 0.5) / 255.0;
        FragColor = vec4(vertexColor.rgb + dither, vertexColor.a);
    } else if (uModelShading == 5) {
        vec3 normal = normalize(surfaceNormal);
        float light = 0.55 + 0.25 * (0.5 + 0.5 * normal.z) + 0.25 * max(dot(normal, normalize(cModelLight)), 0.0);
        vec3 color = vertexColor.rgb * light;
        if (localNormal.z > 0.5) {
            // Ripples, and the sky caught in a streak across the water, which stay put on the water as the view turns
            float across = (localPosition.x + localPosition.y) * 0.5;
            float ripple = inkNoise(localPosition.xy * 2.5 + instanceOrigin.xy * 3.0, 31u);
            float streak = (1.0 - smoothstep(0.04, 0.18, abs(across - 0.25 + 0.12 * ripple))) * smoothstep(0.95, 0.6, abs(localPosition.x - localPosition.y) * 0.5);
            color = mix(color * (0.92 + 0.12 * ripple), vec3(1.0), 0.45 * streak);
        }
        FragColor = vec4(color, vertexColor.a);
    } else if (uInk >= 5) {
        uint seed = inkSeed();
        vec2 p = localPosition.xy;
        float inside;
        if (uInkRound) {
            inside = 1.0 - length(p);
        } else {
            vec2 q = abs(p) - vec2(0.9);
            inside = 0.1 - length(max(q, 0.0)) - min(max(q.x, q.y), 0.0);
        }
        // Drawn by hand: the line wanders slowly in and out round the room, and presses harder in places, but stays
        // inside the shape, which is all there is to draw on
        vec2 around = p / max(length(p), 1e-4);
        float wobble = (inkNoise(around * 1.8 + 3.0, seed) - 0.5) * 0.11 + (inkNoise(around * 6.0 + 8.0, seed + 1u) - 0.5) * 0.03;
        float edge = inside - 0.11 - wobble;
        float pixel = max(fwidth(inside), 1e-5);
        float halfWidth = max(0.03, 0.6 * pixel);
        float line = (1.0 - smoothstep(halfWidth, halfWidth + 1.5 * pixel, abs(edge))) * (0.75 + 0.25 * inkNoise(around * 4.0, seed + 2u));
        float within = smoothstep(-pixel, pixel, edge);
        vec3 inkColor = vertexColor.rgb * 0.28;
        float fill = uInk == 7 ? 0.0 : (uInk == 6 ? 0.55 : 1.0);
        vec4 color = vec4(vertexColor.rgb, fill * within);
        if (uInk == 6) {
            // Hatched across, as a sketch shows a floor over the one looked at
            float across = (p.x - p.y) / 0.3 + inkRandom(ivec2(0), seed);
            float hatch = 1.0 - smoothstep(0.07, 0.07 + 1.5 * fwidth(across), abs(fract(across) - 0.5));
            color = mix(color, vec4(inkColor, 0.8), hatch * within);
        } else if (uInk == 7) {
            line *= step(0.45, fract(atan(p.y, p.x) / 6.2831853 * 14.0));
        }
        color = mix(color, vec4(inkColor, 1.0), line);
        color.a *= paperGrain() * vertexColor.a;
        // Only its top, as drawn on the paper; left until after fwidth(), which a discard before it would upset
        if (localNormal.z < 0.5 || color.a < 0.004) {
            discard;
        }
        FragColor = color;
    } else {
        FragColor = vertexColor;
    }
    // The paper and the sky are the background the fog fades toward already
    if (uFogEnd > uFogStart && uInk != 4 && uModelShading != 4) {
        float fog = clamp((eyeDepth - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0) * uFogMax;
        FragColor.rgb = mix(FragColor.rgb, uFogColor, fog);
    }
    if (uScanlines > 0.0 && mod(gl_FragCoord.y, uScanlinePeriod) < uScanlinePeriod / 3.0) {
        FragColor.rgb *= 1.0 - uScanlines;
    }
}
