layout(location=0) in vec3 a_position;

#ifdef VTX_INPUT_NORMAL
layout(location=1) in vec3 a_normal;
#endif

#ifdef VTX_INPUT_TEXCOORD
layout(location=2) in vec4 a_color;
layout(location=3) in vec2 a_texCoord;
layout(location=4) in float a_texIndex;
#endif

#ifdef VTX_INPUT_BONE_INDEX
layout(location=5) in float a_boneIndex;
#endif

#ifdef VTX_INPUT_COLOR_QUAD
layout(location=6) in vec4 a_colorTopLeft;
layout(location=7) in vec4 a_colorTopRight;
layout(location=8) in vec4 a_colorBottomLeft;
layout(location=9) in vec4 a_colorBottomRight;
#endif
