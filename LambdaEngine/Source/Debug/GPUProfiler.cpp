#include "PreCompiled.h"
#include "Debug/GPUProfiler.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/QueryHeap.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandQueue.h"

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
		RenderSystem::GetDevice()->QueryDeviceFeatures(&desc);
		m_TimestampPeriod = desc.TimestampPeriod;

		CommandQueueProperties prop = {};
		RenderSystem::GetGraphicsQueue()->QueryQueueProperties(&prop);
		m_TimestampValidBits = prop.TimestampValidBits;

		m_TimeUnit = timeUnit;
#endif
	}

	void GPUProfiler::Render(LambdaEngine::Timestamp delta)
	{
#ifdef LAMBDA_DEBUG
		m_TimeSinceUpdate += delta.AsMilliSeconds();

		if (m_TimestampCount != 0 && ImGui::CollapsingHeader("Timestamps") && m_TimeSinceUpdate > 1 / m_UpdateFreq)
		{
			// Enable/disable graph update
			ImGui::Checkbox("Update graphs", &m_EnableGraph);
			for (auto& stage : m_PlotResults)
			{

				// Plot lines
				m_TimeSinceUpdate = 0.0f;
				float average = 0.0f;

				uint32_t index = m_PlotResultsStart;
				for (size_t i = 0; i < m_PlotDataSize; i++)
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
		}

		if (m_pPipelineStatHeap != nullptr && ImGui::CollapsingHeader("Pipeline Stats"))
		{
			// Graphics Pipeline Statistics
			const TArray<std::string> statNames = {
				"Input assembly vertex count        ",
				"Input assembly primitives count    ",
				"Vertex shader invocations          ",
				"Clipping stage primitives processed",
				"Clipping stage primtives output    ",
				"Fragment shader invocations        "
			};

			for (size_t i = 0; i < m_GraphicsStats.GetSize(); i++) {
				std::string caption = statNames[i] + ": %d";
				ImGui::BulletText(caption.c_str(), m_GraphicsStats[i]);
			}
		}
#endif
	}

	void GPUProfiler::Release()
	{
#ifdef LAMBDA_DEBUG
		m_pTimestampHeap->Release();
		m_pPipelineStatHeap->Release();
#endif
	}

	void GPUProfiler::CreateTimestamps(uint32_t listCount)
	{
#ifdef LAMBDA_DEBUG
		// Need two timestamps per list
		m_TimestampCount = listCount * 2;

		QueryHeapDesc createInfo = {};
		createInfo.DebugName = "VulkanProfiler Timestamp Heap";
		createInfo.PipelineStatisticsFlags = 0;
		createInfo.QueryCount = m_TimestampCount;
		createInfo.Type = EQueryType::QUERY_TYPE_TIMESTAMP;

		m_pTimestampHeap = RenderSystem::GetDevice()->CreateQueryHeap(&createInfo);
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

		m_pPipelineStatHeap = RenderSystem::GetDevice()->CreateQueryHeap(&createInfo);

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
					for (size_t i = 0; i < m_PlotDataSize; i++)
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
		pCommandList->Timestamp(m_pTimestampHeap, m_Timestamps[pCommandList].Start, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM);
#endif
	}

	void GPUProfiler::EndTimestamp(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		pCommandList->Timestamp(m_pTimestampHeap, m_Timestamps[pCommandList].End, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM);
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

		size_t timestampCount = 2;
		TArray<uint64_t> results(timestampCount);
		bool res = m_pTimestampHeap->GetResults(m_Timestamps[pCommandList].Start, timestampCount, timestampCount * sizeof(uint64), results.GetData());

		if (res)
		{
			uint64_t start = glm::bitfieldExtract<uint64_t>(results[0], 0, m_TimestampValidBits);
			uint64_t end = glm::bitfieldExtract<uint64_t>(results[1], 0, m_TimestampValidBits);

			if (m_StartTimestamp == 0)
				m_StartTimestamp = start;

			const String& name = m_Timestamps[pCommandList].Name;
			m_Results[name].Start = start;
			m_Results[name].End = end;
			float duration = ((end - start) * m_TimestampPeriod) / (uint64_t)m_TimeUnit;
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
#endif
	}

	void GPUProfiler::ResetTimestamp(CommandList* pCommandList)
	{
#ifdef LAMBDA_DEBUG
		uint32_t firstQuery = m_Timestamps[pCommandList].Start;
		pCommandList->ResetQuery(m_pTimestampHeap, firstQuery, 2);
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

	GPUProfiler* GPUProfiler::Get()
	{
		static GPUProfiler instance;
		return &instance;
	}

	void GPUProfiler::SaveResults()
	{
		/*
			GPU Timestamps are not optimal to represent in a serial fashion that
			chrome://tracing does.
			This feature is therefore possible unnecessary
		*/

#ifdef LAMBDA_DEBUG
		const char* filePath = "GPUResults.txt";

		std::ofstream file;
		file.open(filePath);
		file << "{\"otherData\": {}, \"displayTimeUnit\": \"ms\", \"traceEvents\": [";
		file.flush();

		uint32_t j = 0;
		for (auto& res : m_Results)
		{
			//for (uint32_t i = 0; i < res.second.size(); i++, j++)
			//{
				if (j > 0) file << ",";

				std::string name = "Render Graph Graphics Command List";
				std::replace(name.begin(), name.end(), '"', '\'');

				file << "{";
				file << "\"name\": \"" << name << " " << j << "\",";
				file << "\"cat\": \"function\",";
				file << "\"ph\": \"X\",";
				file << "\"pid\": " << 1 << ",";
				file << "\"tid\": " << 0 << ",";
				file << "\"ts\": " << (((res.second.Start - m_StartTimestamp) * m_TimestampPeriod) / (uint64_t)m_TimeUnit) << ",";
				file << "\"dur\": " << res.second.Duration;
				file << "}";
				j++;
			//}
		}

		file << "]" << std::endl << "}";
		file.flush();
		file.close();
#endif
	}

	std::string GPUProfiler::GetTimeUnitName()
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