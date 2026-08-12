#version 330

in vec3 vertexPosition;

uniform mat4 matProjection;
uniform mat4 matView;

out vec3 fragPosition;

void main()
{
    fragPosition = vertexPosition;
    // Remove a tradução da matriz de visão para que o skybox acompanhe a câmera no infinito
    mat4 viewNoTranslation = mat4(mat3(matView));
    vec4 clipPos = matProjection * viewNoTranslation * vec4(vertexPosition, 1.0);
    
    // Z = W para garantir que a profundidade seja 1.0 (máxima profundidade)
    gl_Position = clipPos.xyww;
}
