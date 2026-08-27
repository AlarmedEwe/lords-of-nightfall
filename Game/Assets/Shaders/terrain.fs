#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Texturas do Terreno (Stencil / Splatmap + 4 materiais)
uniform sampler2D splatmap;     // Stencil / Splatmap (RGBA)
uniform sampler2D texGrass;     // Canal R (Grama)
uniform sampler2D texDirt;      // Canal G (Terra)
uniform sampler2D texSand;      // Canal B (Areia)
uniform sampler2D texPavement;  // Canal A (Pavimento)

uniform vec4 uvScales;          // (x=Grass, y=Dirt, z=Sand, w=Pavement)

// Uniforms de Iluminação
uniform vec3 lightDir;          // Direção da luz do Sol (normalizada)
uniform vec4 lightColor;        // Cor da luz do Sol (RGBA)
uniform vec4 ambient;           // Cor da luz ambiente (RGBA)
uniform vec3 viewPos;           // Posição da câmera

// Uniforms de Sombras (Shadow Mapping)
uniform mat4 lightVP;           // Matriz View-Projection da luz
uniform sampler2D shadowMap;
uniform int shadowMapResolution;

out vec4 finalColor;

void main()
{
    // Amostra do stencil map (splatmap)
    vec4 splat = texture(splatmap, fragTexCoord);
    float totalWeight = splat.r + splat.g + splat.b + splat.a;
    if (totalWeight > 0.001)
    {
        splat /= totalWeight;
    }
    else
    {
        splat = vec4(1.0, 0.0, 0.0, 0.0);
    }

    // Amostragem das 4 texturas com suas respectivas escalas de UV
    vec4 colGrass = texture(texGrass, fragTexCoord * uvScales.x);
    vec4 colDirt  = texture(texDirt,  fragTexCoord * uvScales.y);
    vec4 colSand  = texture(texSand,  fragTexCoord * uvScales.z);
    vec4 colPave  = texture(texPavement, fragTexCoord * uvScales.w);

    // Mistura (blend) linear baseada no stencil
    vec4 blendedColor = colGrass * splat.r + colDirt * splat.g + colSand * splat.b + colPave * splat.a;

    // Iluminação Difusa
    vec3 normal = normalize(fragNormal);
    vec3 l = normalize(-lightDir);
    vec3 v = normalize(viewPos - fragPosition);

    float NdotL = max(dot(normal, l), 0.0);
    vec3 diffuse = lightColor.rgb * NdotL;

    // Shadow Mapping
    vec4 fragPosLightSpace = lightVP * vec4(fragPosition, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    if (projCoords.z <= 1.0 && projCoords.x >= 0.0 && projCoords.x <= 1.0 && projCoords.y >= 0.0 && projCoords.y <= 1.0)
    {
        float currentDepth = projCoords.z;
        float bias = max(0.005 * (1.0 - NdotL), 0.0015);
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

        vec2 border = min(projCoords.xy, 1.0 - projCoords.xy);
        float edgeFactor = smoothstep(0.0, 0.08, min(border.x, border.y));
        shadow *= edgeFactor;
    }

    vec3 lighting = ambient.rgb + diffuse * (1.0 - shadow * 0.65);
    vec3 finalRGB = blendedColor.rgb * lighting;

    // Correção Gamma suave
    finalRGB = pow(finalRGB, vec3(1.0 / 1.8));

    finalColor = vec4(finalRGB, 1.0);
}
