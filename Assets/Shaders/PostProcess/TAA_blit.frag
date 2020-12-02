#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

#define NO_BUFFERS
#include "../Defines.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_HistoryCopy;

layout(location = 0) in vec2 in_TexCoord;
layout(location = 0) out vec4 out_History;
layout(location = 1) out vec4 out_Color;

void main()
{
	// Perform blit
	const vec2 texCoord = in_TexCoord;
	vec4 color 	= texture(u_HistoryCopy, texCoord); 
	out_History	= color;
	out_Color 	= color; 
}