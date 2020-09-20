#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

#include "../Helpers.glsl"
#include "../Defines.glsl"
#include "RayTracingInclude.glsl"

struct SRayHitDescription
{
    vec3    ShadingNormal;
    vec3    GeometricNormal;
    vec2    TexCoord;
    uint    MaterialIndex;
};

layout(binding = 2,     set = BUFFER_SET_INDEX) uniform MaterialParameters  	{ SMaterialParameters val[MAX_UNIQUE_MATERIALS]; }  u_MaterialParameters;

layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_AOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_RoughnessMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 5, set = TEXTURE_SET_INDEX) uniform sampler2D u_MetallicMaps[MAX_UNIQUE_MATERIALS];

layout(buffer_reference, buffer_reference_align = 16) buffer VertexBuffer 
{
    SVertex v[];
};

layout(buffer_reference, buffer_reference_align = 4, scalar) buffer IndexBuffer 
{
    uvec3 t[];
};

layout(shaderRecordEXT) buffer SBTData 
{
    VertexBuffer vertices;
    IndexBuffer indices;
};

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

hitAttributeEXT vec3 attribs;

SRayHitDescription CalculateHitData()
{
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    const uvec3 tri = indices.t[gl_PrimitiveID];

	SVertex v0 = vertices.v[tri.x];
	SVertex v1 = vertices.v[tri.y];
	SVertex v2 = vertices.v[tri.z];

	vec3 T = normalize(v0.Tangent.xyz * barycentricCoords.x + v1.Tangent.xyz * barycentricCoords.y + v2.Tangent.xyz * barycentricCoords.z);
	vec3 N = normalize(v0.Normal.xyz * barycentricCoords.x + v1.Normal.xyz * barycentricCoords.y + v2.Normal.xyz * barycentricCoords.z);

	T = normalize(gl_ObjectToWorldEXT * vec4(T, 0.0f));
	N = normalize(gl_ObjectToWorldEXT * vec4(N, 0.0f));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

    vec2 texCoord = (v0.TexCoord.xy * barycentricCoords.x + v1.TexCoord.xy * barycentricCoords.y + v2.TexCoord.xy * barycentricCoords.z);

    uint materialIndex      = gl_InstanceCustomIndexEXT & 0xFF;
	vec3 shadingNormal      = texture(u_NormalMaps[materialIndex], texCoord).xyz;
	shadingNormal           = normalize(shadingNormal * 2.0f - 1.0f);
	shadingNormal           = TBN * shadingNormal;

    SRayHitDescription hitDescription;
    hitDescription.ShadingNormal    = shadingNormal;
    hitDescription.GeometricNormal  = N;
    hitDescription.TexCoord         = texCoord;
    hitDescription.MaterialIndex    = materialIndex;

    return hitDescription;
}

void main() 
{
   SRayHitDescription hitDescription = CalculateHitData();

   	vec3 hitPos = gl_WorldRayOriginEXT + normalize(gl_WorldRayDirectionEXT) * gl_HitTEXT;

	SMaterialParameters materialParameters = u_MaterialParameters.val[hitDescription.MaterialIndex];

	vec3 sampledAlbedo 		=		texture(u_AlbedoMaps[hitDescription.MaterialIndex],    hitDescription.TexCoord).rgb;
    float sampledAO 		=       texture(u_AOMaps[hitDescription.MaterialIndex],         hitDescription.TexCoord).r;
	float sampledMetallic 	= 		texture(u_MetallicMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;
	float sampledRoughness 	= 		texture(u_RoughnessMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;

    vec3 albedo       		= pow(  materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
	float roughness   		= 		materialParameters.Roughness * sampledRoughness;
    float metallic    		= 		materialParameters.Metallic * sampledMetallic;

	s_PrimaryPayload.HitPosition		= hitPos;
	s_PrimaryPayload.ShadingNormal		= hitDescription.ShadingNormal;
	s_PrimaryPayload.GeometricNormal	= hitDescription.GeometricNormal;
	s_PrimaryPayload.Albedo			    = albedo;
    s_PrimaryPayload.Roughness			= sampledAO;
	s_PrimaryPayload.Roughness			= roughness;
	s_PrimaryPayload.Metallic			= metallic;
	s_PrimaryPayload.Distance			= gl_HitTEXT;
}