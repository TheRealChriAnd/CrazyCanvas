#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"

layout(location = 0) in vec2 in_TexCoords;
layout(location = 1) in flat uint in_AtlasIndex;
layout(location = 2) in vec4 in_EmitterColor;

layout(binding = 0, set = 1) uniform sampler2D u_TextureAtlases[];

layout(location = 0) out vec4 out_ParticleImage;

void main()
{
	vec4 color = texture(u_TextureAtlases[in_AtlasIndex], in_TexCoords);
	color *= in_EmitterColor;
	if(color.a < 0.9f) discard;
	out_ParticleImage = color;
}