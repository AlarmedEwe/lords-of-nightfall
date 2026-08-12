#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Uniforms de Iluminação
uniform vec3 lightDir;      // Direção da luz do Sol (normalizada)
uniform vec4 lightColor;    // Cor da luz do Sol (RGBA)
uniform vec4 ambient;       // Cor da luz ambiente (RGBA)
uniform vec3 viewPos;       // Posição da câmera

// Uniforms do Shadow Mapping
uniform mat4 lightVP;       // Matriz View-Projection da luz
uniform sampler2D shadowMap;
uniform int shadowMapResolution;

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 normal = normalize(fragNormal);
    vec3 l = normalize(-lightDir);
    vec3 v = normalize(viewPos - fragPosition);

    // Iluminação Difusa (Lambertian)
    float NdotL = max(dot(normal, l), 0.0);
    vec3 diffuse = lightColor.rgb * NdotL;

    // Iluminação Especular (Blinn-Phong)
    vec3 h = normalize(l + v);
    float specCo = pow(max(dot(normal, h), 0.0), 32.0);
    vec3 specular = lightColor.rgb * specCo * 0.25;

    // Cálculo do Shadow Mapping (Light Space transformation)
    vec4 fragPosLightSpace = lightVP * vec4(fragPosition, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;

    // Verifica se os fragmentos estão dentro da região coberta pelo shadow map
    if (projCoords.z <= 1.0 && projCoords.x >= 0.0 && projCoords.x <= 1.0 && projCoords.y >= 0.0 && projCoords.y <= 1.0)
    {
        float currentDepth = projCoords.z;
        // Bias adaptativo para evitar self-shadowing no terreno
        float bias = max(0.005 * (1.0 - NdotL), 0.0015);

        // PCF (Percentage-Closer Filtering) 3x3 para bordas de sombras suavizadas
        vec2 texelSize = vec2(1.0 / float(shadowMapResolution));
        float shadowSum = 0.0;

        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                if (currentDepth - bias > pcfDepth)
                {
                    shadowSum += 1.0;
                }
            }
        }
        shadow = shadowSum / 9.0;

        // Suaviza as bordas do frustum de sombras para transição imperceptível
        vec2 border = min(projCoords.xy, 1.0 - projCoords.xy);
        float edgeFactor = smoothstep(0.0, 0.08, min(border.x, border.y));
        shadow *= edgeFactor;
    }

    // Combina Luz Ambiente + (Luz Difusa + Especular) * (1.0 - Sombra)
    vec3 ambientTerm = ambient.rgb;
    vec3 lighting = ambientTerm + (diffuse + specular) * (1.0 - shadow * 0.65);

    vec3 finalRGB = texelColor.rgb * lighting;

    // Correção Gamma leve
    finalRGB = pow(finalRGB, vec3(1.0 / 1.8));

    finalColor = vec4(finalRGB, texelColor.a);
}
