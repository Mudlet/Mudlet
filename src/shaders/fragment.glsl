#version 330 core
in vec4 vertexColor;
in vec2 texCoord;
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
    } else {
        FragColor = vertexColor;
    }
}
