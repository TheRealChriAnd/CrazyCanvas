#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in flat uint	in_MaterialSlot;
layout(location = 1) in vec3		in_WorldPosition;
layout(location = 2) in vec3		in_Normal;
layout(location = 3) in vec3		in_Tangent;
layout(location = 4) in vec3		in_Bitangent;
layout(location = 5) in vec2		in_TexCoord;
layout(location = 6) in vec4		in_ClipPosition;
layout(location = 7) in vec4		in_PrevClipPosition;
layout(location = 8) in flat uint	in_ExtensionIndex;
layout(location = 9) in flat uint	in_InstanceIndex;
layout(location = 10) in vec3 		in_ViewDirection;

layout(push_constant) uniform TeamIndex
{
	uint Index;
} p_TeamIndex;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{ 
	SPerFrameBuffer val; 
} u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) readonly buffer MaterialParameters  	{ SMaterialParameters val[]; }	b_MaterialParameters;
layout(binding = 2, set = BUFFER_SET_INDEX) readonly buffer PaintMaskColors			{ vec4 val[]; }					b_PaintMaskColor;
layout(binding = 3, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_CombinedMaterialMaps[];
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_GBufferAORoughMetalValid;
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_GBufferDepthStencil;
layout(binding = 5, set = TEXTURE_SET_INDEX) uniform sampler2D u_DirLShadowMap;
layout(binding = 6, set = TEXTURE_SET_INDEX) uniform samplerCube u_PointLShadowMap[];

layout(binding = 0, set = DRAW_EXTENSION_SET_INDEX) uniform sampler2D u_PaintMaskTextures[];

layout(location = 0) out vec4 out_Color;

void main()
{
	vec3 normal		= normalize(in_Normal);
	vec3 tangent	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord	= in_TexCoord;

	mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 sampledAlbedo				= texture(u_AlbedoMaps[in_MaterialSlot],			texCoord).rgb;
	vec3 sampledNormal				= texture(u_NormalMaps[in_MaterialSlot],			texCoord).rgb;
	vec3 sampledCombinedMaterial	= texture(u_CombinedMaterialMaps[in_MaterialSlot],	texCoord).rgb;

	vec3 shadingNormal		= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal			= normalize(TBN * normalize(shadingNormal));

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialSlot];

	vec2 currentNDC		= (in_ClipPosition.xy / in_ClipPosition.w) * 0.5f + 0.5f;
	vec2 prevNDC		= (in_PrevClipPosition.xy / in_PrevClipPosition.w) * 0.5f + 0.5f;

	uint serverData				= floatBitsToUint(texture(u_PaintMaskTextures[in_ExtensionIndex], texCoord).r);
	uint clientData				= floatBitsToUint(texture(u_PaintMaskTextures[in_ExtensionIndex], texCoord).g);
	float shouldPaint 			= float((serverData & 0x1) | (clientData & 0x1));

	uint clientTeam				= (clientData >> 1) & 0x7F;
	uint serverTeam				= (serverData >> 1) & 0x7F;
	uint clientPainting			= clientData & 0x1;
	uint team = serverTeam;
	
	if (clientPainting > 0)
		team = clientTeam;

	// TODO: Change this to a buffer input which we can index the team color to
	vec3 teamColor = b_PaintMaskColor.val[team].rgb;

	float backSide = 1.0f - step(0.0f, dot(in_ViewDirection, shadingNormal));
	teamColor = mix(teamColor, teamColor*0.8, backSide);

	// Only render team members and paint on enemy players
	uint enemy = p_TeamIndex.Index;
	bool isPainted = (shouldPaint > 0.5f);
	if(enemy != 0 && !isPainted)
		discard;

	// PBR shading
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;
	SLightsBuffer lightBuffer		= b_LightsBuffer.val;

	vec3 albedo = pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
	vec4 aoRoughMetalValid	= texture(u_GBufferAORoughMetalValid, in_TexCoord);

	albedo = mix(albedo, teamColor, shouldPaint);

	float alpha = isPainted ? 1.0f : 0.6f;
	vec3 colorHDR;
	//1
	if (aoRoughMetalValid.a < 1.0f || aoRoughMetalValid.g == 0.0f)
	{
		//Reinhard Tone-Mapping
		vec3 colorLDR = albedo / (albedo + vec3(1.0f));

		//Gamma Correction
		vec3 finalColor = pow(colorLDR, vec3(1.0f / GAMMA));

		out_Color = vec4(finalColor, alpha);

		return;
	}
	else 
	{
		float ao		= aoRoughMetalValid.r;
		float roughness	= max(0.05f, aoRoughMetalValid.g);
		float metallic	= aoRoughMetalValid.b;
		float depth 	= texture(u_GBufferDepthStencil, in_TexCoord).r;

		SPositions positions    = CalculatePositionsFromDepth(in_TexCoord, depth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
		vec3 N 					= sampledNormal;
		vec3 viewVector			= perFrameBuffer.CameraPosition.xyz - positions.WorldPos;
		float viewDistance		= length(viewVector);
		vec3 V 					= in_ViewDirection;

		vec3 Lo = vec3(0.0f);
		vec3 F0 = vec3(0.04f);
		
		F0 = mix(F0, albedo, metallic);
		// Directional Light
		{
			vec3 L = normalize(lightBuffer.DirL_Direction);
			vec3 H = normalize(V + L);

			vec4 fragPosLight 		= lightBuffer.DirL_ProjView * vec4(positions.WorldPos, 1.0);
			float inShadow 			= DirShadowDepthTest(fragPosLight, N, lightBuffer.DirL_Direction, u_DirLShadowMap);
			vec3 outgoingRadiance    = lightBuffer.DirL_ColorIntensity.rgb * lightBuffer.DirL_ColorIntensity.a;
			vec3 incomingRadiance    = outgoingRadiance * (1.0 - inShadow);

			float NDF   = Distribution(N, H, roughness);
			float G     = Geometry(N, V, L, roughness);
			vec3 F      = Fresnel(F0, max(dot(V, H), 0.0f));

			vec3 nominator      = NDF * G * F;
			float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
			vec3 specular       = nominator / max(denominator, 0.001f);

			vec3 kS = F;
			vec3 kD = vec3(1.0f) - kS;

			kD *= 1.0 - metallic;

			float NdotL = max(dot(N, L), 0.0f);

			Lo += (kD * albedo / PI + specular) * incomingRadiance * NdotL;
		}

		//Point Light Loop
		if (isPainted) {
			for (uint i = 0; i < uint(lightBuffer.PointLightCount); i++)
			{
				SPointLight light = b_LightsBuffer.pointLights[i];

				vec3 L = (light.Position - positions.WorldPos);
				float distance = length(L);
				L = normalize(L);
				vec3 H = normalize(V + L);
				
				float inShadow 			= PointShadowDepthTest(positions.WorldPos, light.Position, viewDistance, N, u_PointLShadowMap[light.TextureIndex], light.FarPlane);
				float attenuation   	= 1.0f / (distance * distance);
				vec3 outgoingRadiance    = light.ColorIntensity.rgb * light.ColorIntensity.a;
				vec3 incomingRadiance    = outgoingRadiance * attenuation * (1.0 - inShadow);
			
				float NDF   = Distribution(N, H, roughness);
				float G     = Geometry(N, V, L, roughness);
				vec3 F      = Fresnel(F0, max(dot(V, H), 0.0f));

				vec3 nominator      = NDF * G * F;
				float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
				vec3 specular       = nominator / max(denominator, 0.001f);

				vec3 kS = F;
				vec3 kD = vec3(1.0f) - kS;

				kD *= 1.0 - metallic;

				float NdotL = max(dot(N, L), 0.0f);

				Lo += (kD * albedo / PI + specular) * incomingRadiance * NdotL;
			}
		}

		
		vec3 ambient    = 0.03f * albedo * ao;
		colorHDR      	= ambient + Lo;
	}

	
	// float luminance = CalculateLuminance(colorHDR);
	// float alpha = isPainted ? 1.0f : 0.6f;
	// //Reinhard Tone-Mapping
	// vec3 colorLDR = colorHDR / (colorHDR + vec3(1.0f));

	// //Gamma Correction
	// vec3 finalColor = pow(colorLDR, vec3(1.0f / GAMMA));

	// out_Color = vec4(finalColor, 1.0f);

	// 5
	out_Color = vec4(colorHDR, alpha);
	// out_Color = vec4(colorHDR, alpha);
}
