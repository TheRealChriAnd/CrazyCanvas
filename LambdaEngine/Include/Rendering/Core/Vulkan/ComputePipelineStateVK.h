#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class ComputePipelineStateVK : public TDeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		ComputePipelineStateVK(const GraphicsDeviceVK* pDevice);
		~ComputePipelineStateVK();

		bool Init(const ComputePipelineStateDesc* pDesc);

		FORCEINLINE VkPipeline GetPipeline() const 
		{
			return m_Pipeline;
		}
		
		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;

		// IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override final
		{
			return EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE;
		}

	private:
		VkPipeline m_Pipeline;
	};
}
