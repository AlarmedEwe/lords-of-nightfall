#version 330

in vec3 fragPosition;

uniform vec3 lightDir; // Direção do sol (normalizada)

out vec4 finalColor;

void main()
{
    vec3 dir = normalize(fragPosition);
    
    // Cores da atmosfera do jogo
    vec3 skyZenith = vec3(0.12, 0.40, 0.85);  // Azul profundo no topo
    vec3 skyHorizon = vec3(0.68, 0.82, 0.96); // Névoa suave na linha do horizonte
    vec3 skyGround = vec3(0.20, 0.35, 0.20);  // Tom terroso abaixo do horizonte

    float h = dir.y;
    vec3 skyColor;

    if (h > 0.0)
    {
        // Gradiente do horizonte ao zênite
        float factor = pow(h, 0.5);
        skyColor = mix(skyHorizon, skyZenith, factor);
    }
    else
    {
        // Gradiente suave para a parte inferior do horizonte
        float factor = pow(clamp(-h, 0.0, 1.0), 0.4);
        skyColor = mix(skyHorizon, skyGround, factor);
    }

    // Disco do Sol e brilho atmosférico em torno da luz solar
    vec3 sDir = normalize(-lightDir);
    float sunDot = max(0.0, dot(dir, sDir));
    
    // Brilho difuso em volta do sol
    float sunGlow = pow(sunDot, 16.0) * 0.4;
    // Disco nítido do sol
    float sunDisc = step(0.9985, sunDot);

    vec3 sunColor = vec3(1.0, 0.95, 0.80);
    skyColor += sunGlow * sunColor + sunDisc * sunColor * 2.0;

    finalColor = vec4(skyColor, 1.0);
}
