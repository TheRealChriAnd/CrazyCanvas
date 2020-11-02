#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{ 
	SPerFrameBuffer val; 
} u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_GBufferPosition;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_GBufferAlbedo;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_GBufferAORoughMetalValid;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_GBufferCompactNormal;
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_GBufferVelocity;

layout(binding = 5, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_DirLShadowMap;
layout(binding = 6, set = TEXTURE_SET_INDEX) uniform samplerCube 	u_PointLShadowMap[];

layout(binding = 7, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_RayTracedRadiance;

layout(location = 0) out vec4 out_Color;

void main()
{
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;
	SLightsBuffer lightBuffer		= b_LightsBuffer.val;

	vec3 albedo				= texture(u_GBufferAlbedo, in_TexCoord).rgb;
	vec4 aoRoughMetalValid	= texture(u_GBufferAORoughMetalValid, in_TexCoord);
	vec3 colorHDR;
	float test;
	if (aoRoughMetalValid.a < 1.0f)
	{
		colorHDR = albedo / (albedo + vec3(1.0f));
		colorHDR = pow(colorHDR, vec3(1.0f / GAMMA));
		out_Color = vec4(colorHDR, CalculateLuminance(colorHDR));
		return;
	}
	else if (aoRoughMetalValid.g > 0.0f)
	{
		float ao		= aoRoughMetalValid.r;
		float roughness	= max(0.05f, aoRoughMetalValid.g);
		float metallic	= aoRoughMetalValid.b;

		vec3 worldPos 		= texture(u_GBufferPosition, in_TexCoord).rgb;
		vec3 N 				= UnpackNormal(texture(u_GBufferCompactNormal, in_TexCoord).xyz);
		vec3 viewVector		= perFrameBuffer.CameraPosition.xyz - worldPos;
		float viewDistance	= length(viewVector);
		vec3 V 				= normalize(viewVector);

		vec3 Lo = vec3(0.0f);
		vec3 F0 = vec3(0.04f);

		F0 = mix(F0, albedo, metallic);
		// Directional Light
		{
			vec3 L = normalize(lightBuffer.DirL_Direction);
			vec3 H = normalize(V + L);

			vec4 fragPosLight 		= lightBuffer.DirL_ProjView * vec4(worldPos, 1.0);
			float inShadow 			= DirShadowDepthTest(fragPosLight, N, lightBuffer.DirL_Direction, u_DirLShadowMap);
			test = inShadow;
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
		for (uint i = 0; i < uint(lightBuffer.PointLightCount); i++)
		{
			SPointLight light = b_LightsBuffer.pointLights[i];

			vec3 L = (light.Position - worldPos);
			float distance = length(L);
			L = normalize(L);
			vec3 H = normalize(V + L);
			
			float inShadow 			= PointShadowDepthTest(worldPos, light.Position, viewDistance, N, u_PointLShadowMap[light.TextureIndex], light.FarPlane);
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
		
		vec3 ambient    = 0.03f * albedo * ao;
		colorHDR      	= ambient + Lo;
	}
	else
	{
		colorHDR = texture(u_RayTracedRadiance, in_TexCoord).rgb;
	}

	float luminance = CalculateLuminance(colorHDR);

	//Reinhard Tone-Mapping
	vec3 colorLDR = colorHDR / (colorHDR + vec3(1.0f));

	//Gamma Correction
	vec3 finalColor = pow(colorLDR, vec3(1.0f / GAMMA));

	out_Color = vec4(finalColor, luminance);
}
