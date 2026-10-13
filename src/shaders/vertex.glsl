#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;

// Per-instance attributes (for instanced rendering)
layout (location = 3) in vec4 aInstanceColor; // Per-instance color
layout (location = 4) in mat4 aInstanceTransform;   // Per-instance transformation matrix (scale -> rotate -> translate), takes locations 4-7

// Texture coordinates (for textured rendering)
layout (location = 8) in vec2 aTexCoord;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMatrix;
uniform bool uUseInstancing = false;
uniform bool uUseTexture = false;
uniform bool uDisableLighting = false;

// A low ambient and one soft key light from high in the north-west: a room's top shows its own color, its sides shade
uniform float uAmbient = 0.5;
uniform vec3 uKeyLightDirection = vec3(-0.3, 0.4, 0.866);
uniform float uKeyLight = 0.6;

out vec4 vertexColor;
out vec2 texCoord;
// How far in front of the camera, for the fog
out float eyeDepth;
// The untransformed template vertex, so a wireframe can find the shape's edges
out vec3 localPosition;
out vec3 localNormal;

void main()
{
    // Determine position and color based on whether we're using instanced rendering
    vec4 finalPos = vec4(aPos, 1.0);
    vec4 finalNormal = vec4(aNormal, 0.0);
    vec4 finalColor = aColor;
    if (uUseInstancing) {
        finalPos = aInstanceTransform * finalPos;
        // Normals need the inverse transpose: the instance transform scales unevenly
        finalNormal = vec4(normalize(transpose(inverse(mat3(aInstanceTransform))) * aNormal), 0.0);
        finalColor = aInstanceColor;
    }

    
    vec3 worldNormal = normalize(uNormalMatrix * vec3(finalNormal));

    if (uDisableLighting) {
        // Pass through color without lighting (for labels/billboards)
        vertexColor = finalColor;
    } else {
        float diffuse = max(dot(worldNormal, normalize(uKeyLightDirection)), 0.0);
        vertexColor = vec4(finalColor.rgb * min(uAmbient + uKeyLight * diffuse, 1.0), finalColor.a);
    }
    
    // Pass texture coordinates to fragment shader
    texCoord = aTexCoord;
    localPosition = aPos;
    localNormal = aNormal;
    
    gl_Position = uMVP * finalPos;
    eyeDepth = gl_Position.w;
}
