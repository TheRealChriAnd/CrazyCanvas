#include "PreCompiled.h"
#include "Debug/GPUProfiler.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/QueryHeap.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/GraphicsTypes.h"

#include <glm/glm.hpp>
#include <imgui.h>

#include <fstream>
#include <sstream>

namespace LambdaEngine
{
	GPUProfiler::GPUProfiler() : m_TimeUnit(TimeUnit::MILLI), m_PlotDataSize(100), m_UpdateFreq(1.0f)
	{
	}

	GPUProfiler::~GPUProfiler()
	{
	}

	void GPUProfiler::Init(TimeUnit timeUnit)
	{
#ifdef LAMBDA_DEBUG
		GraphicsDeviceFeatureDesc desc = {};
		RenderAPI::GetDevice()->QueryDeviceFeatures(&desc);
		m_TimestampPeriod = desc.TimestampPeriod;

		CommandQueueProperties prop = {};
		RenderAPI::GetGraphicsQueue()->QueryQueueProperties(&prop);
		m_TimestampValidBits = prop.TimestampValidBits;

		m_TimeUnit = timeUnit;

#endif
		uint32 statCount = 0;
		RenderAPI::GetDevice()->QueryDeviceMemoryStatistics(&statCount, m_MemoryStats);
		m_MemoryStats.Resize(statCount);
	}

	void GPUProfiler::Tick(LambdaEngine::Timestamp delta)
	{
		// Update timings
		m_TimeSinceUpdate += delta.AsSeconds();

		// Get memory statistics
		if (m_TimeSinceUpdate > 1 / m_UpdateFreq)
		{
			uint32 statCount = m_MemoryStats.GetSize();
			RenderAPI::GetDevice()->QueryDeviceMemoryStatistics(&statCount, m_MemoryStats);

			// Update average and peak GPU memory
			uint64 memoryUsage = 0;
			for (auto& stats : m_MemoryStats)
			{
				if (stats.MemoryType == EMemoryType::MEMORY_TYPE_GPU)
				{
					memoryUsage += stats.TotalBytesAllocated;
				}
			}

			if (memoryUsage > m_PeakDeviceMemory)
				m_PeakDeviceMemory = memoryUsage;

			// Calculate moving average
			m_AverageDeviceMemory = (memoryUsage + m_AverageCount * m_AverageDeviceMemory) / (m_AverageCount + 1);
			m_AverageCount++;
		}
	}

	void GPUProfiler::Render()
	{
		if (ImGui::CollapsingHeader("GPU Statistics", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10.0f);
			// Profiler (Which has the instance of GPUProfiler) begins the ImGui window

			// Memory display
			static const char* items[] = { "B", "KB", "MB", "GB" };
			static int itemSelected = 2;
			static float byteDivider = 1;
			ImGui::Combo("Memory suffix GPU", &itemSelected, items, 4, 4);
			if (itemSelected == 0) { byteDivider = 1.f; }
			if (itemSelected == 1) { byteDivider = 1024.f; }
			if (itemSelected == 2) { byteDivider = 1024.f * 1024.f; }
			if (itemSelected == 3) { byteDivider = 1024.f * 1024.f * 1024.f; }

			for (uint32 i = 0; i < m_MemoryStats.GetSize(); i++)
			{
				ImGui::Text(m_MemoryStats[i].MemoryTypeName.c_str());
				char buf[64];
				float32 percentage = (float32)(m_MemoryStats[i].TotalBytesAllocated / (float64)m_MemoryStats[i].TotalBytesReserved);
				sprintf(buf, "%.3f/%.3f (%s)", m_MemoryStats[i].TotalBytesAllocated / byteDivider, m_MemoryStats[i].TotalBytesReserved / byteDivider, items[itemSelected]);
				ImGui::ProgressBar(percentage, ImVec2(-1.0f, 0.0f), buf);
			}

			ImGui::Text("Average Device Memory Usage: %.3f", m_AverageDeviceMemory / byteDivider);
			ImGui::Text("Peak Device Memory Usage: %.3f", m_PeakDeviceMemory / byteDivider);

			if(ImGui::Button("Reset Memory Counts"))
			{
				m_PeakDeviceMemory		= 0;
				m_AverageCount			= 0;
				m_AverageDeviceMemory	= 0;
			}

#ifdef LAMBDA_DEBUG
			// Timestamp display
			if (m_TimestampCount != 0 && ImGui::CollapsingHeader("Timestamps"))
			{
				ImGui::Indent(10.0f);
				ImGui::SliderFloat("Update frequency", &m_UpdateFreq, 0.10f, 60.0f);

				// Enable/disable graph update
				ImGui::Checkbox("Update graphs", &m_EnableGraph);
				for (auto& stage : m_PlotResults)
				{

					// Plot lines
					m_TimeSinceUpdate = 0.0f;
					float average = 0.0f;

					for (uint32 i = 0; i < m_PlotDataSize; i++)
					{
						average += stage.Results[i];
					}
					average /= m_PlotDataSize;

					std::ostringstream overlay;

					overlay.precision(2);
					overlay << "Average: " << std::fixed << average << GetTimeUnitName();

					ImGui::Text(stage.Name.c_str());
					ImGui::PlotLines("", stage.Results.GetData(), (int)m_PlotDataSize, m_PlotResultsStart, overlay.str().c_str(), 0.f, m_CurrentMaxDuration[stage.Name], { 0, 80 });
				}
				ImGui::Unindent(10.0f);
			}

			// Graphics pipeline statistics display
			if (m_pPipelineStatHeap != nullptr && ImGui::CollapsingHeader("Pipeline Stats"))
			{
				ImGui::Indent(10.0f);
				// Graphics Pipeline Statistics
				const TArray<std::string> statNames = {
					"Input assembly vertex count        ",
					"Input assembly primitives count    ",
					"Vertex shader invocations          ",
					"Clipping stage primitives processed",
					"Clipping stage primtives output    ",
					"Fragment shader invocations        "
				};

				for (uint32 i = 0; i < m_GraphicsStats.GetSize(); i++) {
					std::string caption = statNames[i] + ": %d";
					ImGui::BulletText(caption.c_str(), m_GraphicsStats[i]);
				}
				ImGui::Unindent(10.0f);
			}
			ImGui::Unindent(10.0f);
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
#endif
		}
	}

	void GPUProfiler::Release()
	{
#ifdef LAMBDA_DEBUG
		SAFERELEASE(m_pTimestampHeap);
		SAFERELEASE(m_pPipelineStatHeap);

		m_Timestamps.clear();
		m_TimestampCount		= 0;
		m_NextIndex				= 0;
		m_TimestampValidBits	= 0;
		m_TimestampPeriod		= 0.0f;

		m_Results.clear();
		m_PlotResults.Clear();
		m_PlotResultsStart		= 0;
		m_CurrentMaxDuration.clear();
		m_TimeSinceUpdate		= 0.0f;
		m_EnableGraph			= true;

		m_ShouldGetTimestamps.clear();

		m_GraphicsStats.Clear();
#endif
	}

	void GPUProfiler::CreateTimestamps(uint32 listCount)
	{
#ifdef LAMBDA_DEBUG
		// Need two timestamps per list
		m_TimestampCount = listCount * 2;

		QueryHeapDesc createInfo = {};
		createInfo.DebugName = "VulkanProfiler Timestamp Heap";
		createInfo.PipelineStatisticsFlags = 0;
		createInfo.QueryCount = m_TimestampCount;
		createInfo.Type = EQueryType::QUERY_TYPE_TIMESTAMP;

		m_pTimestampHeap = RenderAPI::GetDevice()->CreateQueryHeap(&createInfo);
#endif
	}

	void GPUProfiler::CreateGraphicsPipelineStats()
	{
#ifdef LAMBDA_DEBUG
		QueryHeapDesc createInfo = {};
		createInfo.DebugName = "VulkanProfiler Graphics Pipeline Statistics Heap";
		createInfo.PipelineStatisticsFlags =
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_VERTICES |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_PRIMITIVES |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_VERTEX_SHADER_INVOCATIONS |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_INVOCATIONS |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_PRIMITIVES |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_FRAGMENT_SHADER_INVOCATIONS;
		createInfo.QueryCount = 6;
		createInfo.Type = EQueryType::QUERY_TYPE_PIPELINE_STATISTICS;

		m_pPipelineStatHeap = RenderAPI::GetDevice()->CreateQueryHeap(&createInfo);

		m_GraphicsStats.Resize(createInfo.QueryCount);
#endif
	}

	void GPUProfiler::AddTimestamp(CommandList* pCommandList, const String& name)
	{
#ifdef LAMBDA_DEBUG
		if (m_Timestamps.find(pCommandList) == m_Timestamps.end())
		{
			m_Timestamps[pCommandList].pCommandList = pCommandList;
			m_Timestamps[pCommandList].Start		= m_NextIndex++;
			m_Timestamps[pCommandList].End			= m_NextIndex++;
			m_Timestamps[pCommandList].Name			= name;

			auto plotResultIt = std::find_if(m_PlotResults.Begin(), m_PlotResults.End(), [name](const PlotResult& plotResult) { return name == plotResult.Name; });
			if (plotResultIt == m_PlotResults.End())
			{
				PlotResult plotResult = {};
				plotResult.Name = name;
				plotResult.Results.Resize(m_PlotDataSize);
					for (uint32 i = 0; i < m_PlotDataSize; i++)
						plotResult.Results[i] = 0.0f;

				m_PlotResults.PushBack(plotResult);
			}
		}
#endif
	}

	void GPUProfiler::StartTimestamp(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		// Assume VK_PIPELINE_STAGE_TOP_OF_PIPE or VK_PIPELINE_STAGE_BOTTOM_OF_PIPE;
		pCommandList->Timestamp(m_pTimestampHeap, (uint32)m_Timestamps[pCommandList].Start, FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM);
#endif
	}

	void GPUProfiler::EndTimestamp(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		pCommandList->Timestamp(m_pTimestampHeap, (uint32)m_Timestamps[pCommandList].End, FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM);
#endif
	}

	void GPUProfiler::GetTimestamp(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		// Don't get the first time to make sure the timestamps are on the GPU and are ready
		if (m_ShouldGetTimestamps.find(pCommandList) == m_ShouldGetTimestamps.end())
		{
			m_ShouldGetTimestamps[pCommandList] = false;
			return;
		}
		else if (!m_ShouldGetTimestamps[pCommandList])
		{
			m_ShouldGetTimestamps[pCommandList] = true;
			return;
		}

		uint32 timestampCount = 2;
		TArray<QueryHeapAvailabilityResult> results(timestampCount);
		bool res = m_pTimestampHeap->GetResultsAvailable((uint32)m_Timestamps[pCommandList].Start, timestampCount, timestampCount * sizeof(QueryHeapAvailabilityResult), results.GetData());

		if (res)
		{
			for (auto& result : results)
			{
				if (result.Availability == 0)
				{
					// If any value of the pair is not available yet, do not use the values
					// TODO: Try to gather result later in tick as a backup?
					return;
				}
			}

			uint64 start = glm::bitfieldExtract<uint64>(results[0].Result, 0, m_TimestampValidBits);
			uint64 end = glm::bitfieldExtract<uint64>(results[1].Result, 0, m_TimestampValidBits);

			const String& name = m_Timestamps[pCommandList].Name;
			m_Results[name].Start = start;
			m_Results[name].End = end;
			float duration = ((end - start) * m_TimestampPeriod) / (uint64)m_TimeUnit;
			m_Results[name].Duration = duration;

			if (duration > m_CurrentMaxDuration[name])
				m_CurrentMaxDuration[name] = duration;

			if (m_EnableGraph)
			{
				auto plotResultIt = std::find_if(m_PlotResults.Begin(), m_PlotResults.End(), [name](const PlotResult& plotResult) { return name == plotResult.Name; });
				if (plotResultIt != m_PlotResults.End())
				{
					plotResultIt->Results[m_PlotResultsStart] = duration;
				}
				m_PlotResultsStart = (m_PlotResultsStart + 1) % m_PlotDataSize;
			}
		}
		else
		{
			LOG_DEBUG("Failed to get available results with %d number of timestamps", timestampCount);
		}
#endif
	}

	void GPUProfiler::ResetTimestamp(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		pCommandList->ResetQuery(m_pTimestampHeap, (uint32)m_Timestamps[pCommandList].Start, 2);
#endif
	}

	void GPUProfiler::ResetAllTimestamps(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		pCommandList->ResetQuery(m_pTimestampHeap, 0, m_TimestampCount);
#endif
	}

	void GPUProfiler::StartGraphicsPipelineStat(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		pCommandList->BeginQuery(m_pPipelineStatHeap, 0);
#endif
	}

	void GPUProfiler::EndGraphicsPipelineStat(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		pCommandList->EndQuery(m_pPipelineStatHeap, 0);
#endif
	}

	void GPUProfiler::GetGraphicsPipelineStat()
	{
#ifdef LAMBDA_DEBUG
			m_pPipelineStatHeap->GetResults(0, 1, m_GraphicsStats.GetSize() * sizeof(uint64), m_GraphicsStats.GetData());
#endif
	}

	void GPUProfiler::ResetGraphicsPipelineStat(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		pCommandList->ResetQuery(m_pPipelineStatHeap, 0, 6);
#endif
	}

	uint64 GPUProfiler::GetAverageDeviceMemory() const
	{
		return m_AverageDeviceMemory;
	}

	uint64 GPUProfiler::GetPeakDeviceMemory() const
	{
		return m_PeakDeviceMemory;
	}

	GPUProfiler* GPUProfiler::Get()
	{
		static GPUProfiler instance;
		return &instance;
	}

	std::string GPUProfiler::GetTimeUnitName() const
	{
		std::string buf;
		switch (m_TimeUnit) {
		case TimeUnit::MICRO:
			buf = "us";
			break;
		case TimeUnit::MILLI:
			buf = "ms";
			break;
		case TimeUnit::NANO:
			buf = "ns";
			break;
		case TimeUnit::SECONDS:
			buf = "s";
			break;
		}
		return buf;
	}
}