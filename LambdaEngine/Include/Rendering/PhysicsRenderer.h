#pragma once

#include "RenderGraphTypes.h"
#include "ICustomRenderer.h"

namespace LambdaEngine
{
	class CommandAllocator;
	class DeviceAllocator;
	class GraphicsDevice;
	class PipelineLayout;
	class DescriptorHeap;
	class DescriptorSet;
	class PipelineState;
	class CommandList;
	class TextureView;
	class CommandList;
	class RenderPass;
	class Texture;
	class Sampler;
	class Shader;
	class Buffer;
	class Window;

	/*
	* ImGuiRendererDesc
	*/
	struct PhysicsRendererDesc
	{
		uint32 BackBufferCount = 0;
		uint32 VerticiesBufferSize = 0;
	};

	struct VertexData
	{
		glm::vec4 Position;
		glm::vec4 Color;
	};

	class PhysicsRenderer : public ICustomRenderer
	{
	public:
		PhysicsRenderer();
		virtual ~PhysicsRenderer();

		bool init(GraphicsDevice* pGraphicsDevice, const PhysicsRendererDesc* pDesc);

		// Custom Renderer implementations
		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
		virtual void PreBuffersDescriptorSetWrite() override final;
		virtual void PreTexturesDescriptorSetWrite() override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure) override final;
		virtual void Render(
			CommandAllocator* pGraphicsCommandAllocator,
			CommandList* pGraphicsCommandList,
			CommandAllocator* pComputeCommandAllocator,
			CommandList* pComputeCommandList,
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppPrimaryExecutionStage,
			CommandList** ppSecondaryExecutionStage)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_INPUT; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }
		FORCEINLINE virtual const String& GetName() const override
		{
			static String name = RENDER_GRAPH_PHYSICS_DEBUG_STAGE;
			return name;
		}


		// DebugDraw implementations
		void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
		void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& fromColor, const glm::vec3& toColor);
		//void DrawSphere(const btVecglm::vec3tor3& p, float radius, const glm::vec3& color);
		//void DrawTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color, float alpha);
		void DrawContactPoint(const glm::vec3& PointOnB, const glm::vec3& normalOnB, float distance, int lifeTime, const glm::vec3& color);

	public:
		static PhysicsRenderer* Get();

	private:
		bool CreateCopyCommandList();
		bool CreateBuffers(uint32 verticiesBufferSize);
		//bool CreateTextures();
		//bool CreateSamplers();
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc, RenderPassAttachmentDesc* pDepthStencilAttachmentDesc);
		bool CreatePipelineState();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader);

	private:
		int m_DebugMode = 1;

		const GraphicsDevice*	m_pGraphicsDevice = nullptr;

		TArray<VertexData> m_Verticies;

		TArray<TSharedRef<const TextureView>>	m_BackBuffers;
		TSharedRef<const TextureView>			m_DepthStencilBuffer;

		TSharedRef<CommandAllocator>	m_CopyCommandAllocator	= nullptr;
		TSharedRef<CommandList>			m_CopyCommandList		= nullptr;

		uint64						m_PipelineStateID	= 0;
		TSharedRef<PipelineLayout>	m_PipelineLayout	= nullptr;
		TSharedRef<DescriptorHeap>	m_DescriptorHeap	= nullptr;
		TSharedRef<DescriptorSet>	m_DescriptorSet		= nullptr;

		GUID_Lambda m_VertexShaderGUID	= 0;
		GUID_Lambda m_PixelShaderGUID	= 0;

		TSharedRef<RenderPass> m_RenderPass = nullptr;

		TArray<TSharedRef<Buffer>> m_UniformCopyBuffers;
		TSharedRef<Buffer> m_UniformBuffer	= nullptr;

		THashTable<String, TArray<TSharedRef<DescriptorSet>>>		m_BufferResourceNameDescriptorSetsMap;
		THashTable<GUID_Lambda, THashTable<GUID_Lambda, uint64>>	m_ShadersIDToPipelineStateIDMap;

	private:
		static PhysicsRenderer* s_pInstance;
	};

}