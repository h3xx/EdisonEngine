layout(location=11) IN_OUT GeometryPipelineInterface {
    vec2 texCoord;
    vec4 color;
    flat float texIndex;
    vec3 vertexPos;
    vec3 vertexPosLight1;
    vec3 vertexPosLight2;
    vec3 vertexPosLight3;
    vec3 vertexPosLight4;
    vec3 vertexPosLight5;
    vec3 vertexPosWorld;
    vec3 normal;
    vec3 ssaoNormal;
} gpi;
