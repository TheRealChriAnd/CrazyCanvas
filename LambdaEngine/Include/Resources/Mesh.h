#pragma once
#include "LambdaEngine.h"

#include "Math/Math.h"

#include "Rendering/Core/API/Buffer.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4324) //Disable alignment warning
#endif

namespace LambdaEngine
{
	struct Vertex
	{
		alignas(16) glm::vec3 Position;
		alignas(16) glm::vec3 Normal;
		alignas(16) glm::vec3 Tangent;
		alignas(16) glm::vec2 TexCoord;

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position && Normal == other.Normal && Tangent == other.Tangent && TexCoord == other.TexCoord;
		}
	};

	struct Meshlet
	{
		uint32 VertCount;
		uint32 VertOffset;
		uint32 PrimCount;
		uint32 PrimOffset;
	};

	struct Mesh
	{
		using IndexType = uint32;

		~Mesh()
		{
			SAFEDELETE_ARRAY(pVertexArray);
			SAFEDELETE_ARRAY(pIndexArray);
			SAFEDELETE_ARRAY(pMeshletArray);
		}

		Vertex* pVertexArray	= nullptr;
		IndexType* pIndexArray	= nullptr;
		Meshlet* pMeshletArray	= nullptr;
		uint32 VertexCount		= 0;
		uint32 IndexCount		= 0;
		uint32 MeshletCount		= 0;
	};

	class MeshFactory
	{
	public:
		static Mesh* CreateQuad();
	};
}

namespace std
{
	template<> struct hash<LambdaEngine::Vertex>
	{
		size_t operator()(LambdaEngine::Vertex const& vertex) const
		{
			return
				((hash<glm::vec3>()(vertex.Position) ^
				 (hash<glm::vec3>()(vertex.Normal) << 1)) >> 1) ^
				 (hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}
