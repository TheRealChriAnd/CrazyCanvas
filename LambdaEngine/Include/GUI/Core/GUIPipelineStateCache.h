#pragma once

#include "Rendering/Core/API/PipelineState.h"

#include "GUI/Core/GUIHelpers.h"

#include "NsRender/RenderDevice.h"

namespace LambdaEngine
{
	struct RenderPassAttachmentDesc;
	class PipelineState;
	class GUIRenderTarget;

	constexpr const uint32 NUM_NON_DYNAMIC_VARIATIONS		= 4 * 2 * 2 * 2; //stencilMode(4) * colorEnable(2) * blendEnable(2) * tiled(2)
	constexpr const uint32 NUM_PIPELINE_STATE_VARIATIONS	= NUM_NON_DYNAMIC_VARIATIONS;

	class GUIPipelineStateCache
	{
		struct PipelineVariations
		{
			PipelineState* ppVariations[NUM_PIPELINE_STATE_VARIATIONS];
		};

	public:
		DECL_STATIC_CLASS(GUIPipelineStateCache);

		static bool Init(RenderPassAttachmentDesc* pBackBufferAttachmentDesc);
		static bool Release();

		static PipelineState* GetPipelineState(uint32 index, uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled, const NoesisShaderData& shaderData);

		FORCEINLINE static const PipelineLayout* GetPipelineLayout() { return s_pPipelineLayout; }

	private:
		static bool InitPipelineLayout();
		static bool InitPipelineState(uint32 index, uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled);
		static bool InitPipelineState(uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled, PipelineState** ppPipelineState, const NoesisShaderData& shaderData);

		static uint32 CalculateSubIndex(uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled);

	private:
		static TArray<PipelineVariations> s_PipelineStates;
		static RenderPass*		s_pDummyRenderPass;
		static RenderPass*		s_pTileDummyRenderPass;
		static PipelineLayout*	s_pPipelineLayout;
	};
}