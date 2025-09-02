#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMatrix;

uniform vec3 uLight0Pos = vec3(5000.0, 4000.0, 1000.0);
uniform vec3 uLight1Pos = vec3(5000.0, 1000.0, 1000.0);
uniform vec3 uLight0Diffuse = vec3(0.507, 0.507, 0.507);
uniform vec3 uLight1Diffuse = vec3(0.501, 0.901, 0.501);
uniform vec3 uLight0Ambient = vec3(0.8, 0.8, 0.8);
uniform vec3 uLight1Ambient = vec3(0.4501, 0.4501, 0.4501);

out vec4 vertexColor;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vec3 worldNormal = normalize(uNormalMatrix * aNormal);
    
    vec3 ambient = uLight0Ambient + uLight1Ambient;
    
    vec3 lightDir0 = normalize(uLight0Pos);
    float diff0 = max(dot(worldNormal, lightDir0), 0.0);
    vec3 diffuse0 = diff0 * uLight0Diffuse;
    
    vec3 lightDir1 = normalize(uLight1Pos);
    float diff1 = max(dot(worldNormal, lightDir1), 0.0);
    vec3 diffuse1 = diff1 * uLight1Diffuse;
    
    vec3 lighting = ambient + diffuse0 + diffuse1;
    lighting = clamp(lighting, 0.0, 1.0);
    
    vec3 materialAmbientDiffuse = aColor.rgb;
    
    vec3 ambientContrib = (uLight0Ambient + uLight1Ambient) * materialAmbientDiffuse;
    vec3 diffuseContrib = (diffuse0 + diffuse1) * materialAmbientDiffuse;
    
    vec3 finalColor = ambientContrib + diffuseContrib;
    
    vertexColor = vec4(finalColor, aColor.a);
    
    gl_Position = uMVP * vec4(aPos, 1.0);
}
