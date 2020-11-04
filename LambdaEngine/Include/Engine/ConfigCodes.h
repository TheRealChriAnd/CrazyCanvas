#pragma once
#include "LambdaEngine.h"
#include "Containers/THashTable.h"
#include "Containers/String.h"

namespace LambdaEngine
{
	/*
	* EConfigOption
	*/
	enum EConfigOption : uint8
	{
		CONFIG_OPTION_UNKNOWN = 0,

		CONFIG_OPTION_WINDOW_SIZE = 1,
		CONFIG_OPTION_VOLUME_MASTER = 2,
		CONFIG_OPTION_FIXED_TIMESTEMP = 3,
		CONFIG_OPTION_NETWORK_PORT = 4,
		CONFIG_OPTION_RAY_TRACING = 5,
		CONFIG_OPTION_MESH_SHADER = 6,
		CONFIG_OPTION_SHOW_RENDER_GRAPH = 7,
		CONFIG_OPTION_RENDER_GRAPH_NAME = 8,
		CONFIG_OPTION_LINE_RENDERER = 9,
		CONFIG_OPTION_SHOW_DEMO = 10,
		CONFIG_OPTION_DEBUGGING = 11,
		CONFIG_OPTION_CAMERA_FOV = 12,
		CONFIG_OPTION_CAMERA_NEAR_PLANE = 13,
		CONFIG_OPTION_CAMERA_FAR_PLANE = 14,
		CONFIG_OPTION_STREAM_PHYSX = 15,
		CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG = 16,
		CONFIG_OPTION_NETWORK_PROTOCOL = 17,
	};

	/*
	* Helpers
	*/
	inline const char* ConfigOptionToString(EConfigOption configOption)
	{
		switch (configOption)
		{
			case CONFIG_OPTION_WINDOW_SIZE:					return "CONFIG_OPTION_WINDOW_SIZE";
			case CONFIG_OPTION_VOLUME_MASTER:				return "CONFIG_OPTION_VOLUME_MASTER";
			case CONFIG_OPTION_FIXED_TIMESTEMP:				return "CONFIG_OPTION_FIXED_TIMESTEMP";
			case CONFIG_OPTION_NETWORK_PORT:				return "CONFIG_OPTION_NETWORK_PORT";
			case CONFIG_OPTION_RAY_TRACING:					return "CONFIG_OPTION_RAY_TRACING";
			case CONFIG_OPTION_MESH_SHADER:					return "CONFIG_OPTION_MESH_SHADER";
			case CONFIG_OPTION_SHOW_RENDER_GRAPH:			return "CONFIG_OPTION_SHOW_RENDER_GRAPH";
			case CONFIG_OPTION_RENDER_GRAPH_NAME:			return "CONFIG_OPTION_RENDER_GRAPH_NAME";
			case CONFIG_OPTION_LINE_RENDERER:				return "CONFIG_OPTION_LINE_RENDERER";
			case CONFIG_OPTION_SHOW_DEMO:					return "CONFIG_OPTION_SHOW_DEMO";
			case CONFIG_OPTION_DEBUGGING:					return "CONFIG_OPTION_DEBUGGING";
			case CONFIG_OPTION_CAMERA_FOV:					return "CONFIG_OPTION_CAMERA_FOV";
			case CONFIG_OPTION_CAMERA_NEAR_PLANE:			return "CONFIG_OPTION_CAMERA_NEAR_PLANE";
			case CONFIG_OPTION_CAMERA_FAR_PLANE:			return "CONFIG_OPTION_CAMERA_FAR_PLANE";
			case CONFIG_OPTION_STREAM_PHYSX:				return "CONFIG_OPTION_STREAM_PHYSX";
			case CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG:	return "CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG";
			case CONFIG_OPTION_NETWORK_PROTOCOL:			return "CONFIG_OPTION_NETWORK_PROTOCOL";
			default:										return "CONFIG_OPTION_UNKNOWN";
		}
	}

	inline const EConfigOption StringToConfigOption(String str)
	{
		static const THashTable<String, EConfigOption> configMap = {
			{"CONFIG_OPTION_WINDOW_SIZE", EConfigOption::CONFIG_OPTION_WINDOW_SIZE},
			{"CONFIG_OPTION_VOLUME_MASTER", EConfigOption::CONFIG_OPTION_VOLUME_MASTER},
			{"CONFIG_OPTION_FIXED_TIMESTEMP", EConfigOption::CONFIG_OPTION_FIXED_TIMESTEMP},
			{"CONFIG_OPTION_NETWORK_PORT", EConfigOption::CONFIG_OPTION_NETWORK_PORT},
			{"CONFIG_OPTION_RAY_TRACING", EConfigOption::CONFIG_OPTION_RAY_TRACING},
			{"CONFIG_OPTION_MESH_SHADER", EConfigOption::CONFIG_OPTION_MESH_SHADER},
			{"CONFIG_OPTION_SHOW_RENDER_GRAPH", EConfigOption::CONFIG_OPTION_SHOW_RENDER_GRAPH},
			{"CONFIG_OPTION_RENDER_GRAPH_NAME", EConfigOption::CONFIG_OPTION_RENDER_GRAPH_NAME},
			{"CONFIG_OPTION_LINE_RENDERER", EConfigOption::CONFIG_OPTION_LINE_RENDERER},
			{"CONFIG_OPTION_SHOW_DEMO", EConfigOption::CONFIG_OPTION_SHOW_DEMO},
			{"CONFIG_OPTION_DEBUGGING", EConfigOption::CONFIG_OPTION_DEBUGGING},
			{"CONFIG_OPTION_CAMERA_FOV", EConfigOption::CONFIG_OPTION_CAMERA_FOV},
			{"CONFIG_OPTION_CAMERA_NEAR_PLANE", EConfigOption::CONFIG_OPTION_CAMERA_NEAR_PLANE},
			{"CONFIG_OPTION_CAMERA_FAR_PLANE", EConfigOption::CONFIG_OPTION_CAMERA_FAR_PLANE},
			{"CONFIG_OPTION_STREAM_PHYSX", EConfigOption::CONFIG_OPTION_STREAM_PHYSX},
			{"CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG", EConfigOption::CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG},
			{"CONFIG_OPTION_NETWORK_PROTOCOL", EConfigOption::CONFIG_OPTION_NETWORK_PROTOCOL},
		};

		auto itr = configMap.find(str);
		if (itr != configMap.end()) {
			return itr->second;
		}
		return EConfigOption::CONFIG_OPTION_UNKNOWN;
	}
}