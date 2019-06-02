attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texCoord;
attribute vec3 a_color;

uniform mat4 u_modelMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

out vec2 v_texCoord;
out vec3 v_color;
out vec3 v_vertexPos;
out vec3 v_normal;

void main()
{
    vec4 tmp = u_modelViewMatrix * vec4(a_position, 1);
    gl_Position = u_projectionMatrix * tmp;
    v_texCoord = a_texCoord;
    v_color = a_color;

    v_vertexPos = (u_modelMatrix * vec4(a_position, 1)).xyz;
    v_normal = normalize((u_modelMatrix * vec4(a_normal, 0)).xyz);
}
