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

		CONFIG_OPTION_WINDOW_SIZE				= 1,
		CONFIG_OPTION_FULLSCREEN				= 2,
		CONFIG_OPTION_VOLUME_MASTER				= 3,
		CONFIG_OPTION_FIXED_TIMESTEMP			= 4,
		CONFIG_OPTION_NETWORK_PORT				= 5,
		CONFIG_OPTION_RAY_TRACING 				= 6,
		CONFIG_OPTION_MESH_SHADER				= 7,
		CONFIG_OPTION_SHOW_RENDER_GRAPH			= 8,
		CONFIG_OPTION_RENDER_GRAPH_NAME			= 9,
		CONFIG_OPTION_LINE_RENDERER				= 10,
		CONFIG_OPTION_SHOW_DEMO					= 11,
		CONFIG_OPTION_DEBUGGING					= 12,
		CONFIG_OPTION_CAMERA_FOV				= 13,
		CONFIG_OPTION_CAMERA_NEAR_PLANE			= 14,
		CONFIG_OPTION_CAMERA_FAR_PLANE			= 15,
		CONFIG_OPTION_STREAM_PHYSX				= 16,
		CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG	= 17,
		CONFIG_OPTION_NETWORK_PROTOCOL			= 18,
		CONFIG_OPTION_NETWORK_PING_SYSTEM		= 19,
		CONFIG_OPTION_INLINE_RAY_TRACING		= 20,
		CONFIG_OPTION_GLOSSY_REFLECTIONS		= 21,
		CONFIG_OPTION_REFLECTIONS_SPP			= 22,
		CONFIG_OPTION_RAY_TRACED_SHADOWS		= 23,
		CONFIG_OPTION_VOLUME_MUSIC				= 24,
		CONFIG_OPTION_AA						= 25,
	};

	/*
	* Helpers
	*/
	inline const char* ConfigOptionToString(EConfigOption configOption)
	{
		switch (configOption)
		{
			case CONFIG_OPTION_WINDOW_SIZE:					return "CONFIG_OPTION_WINDOW_SIZE";
			case CONFIG_OPTION_FULLSCREEN:					return "CONFIG_OPTION_FULLSCREEN";
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
			case CONFIG_OPTION_NETWORK_PING_SYSTEM:			return "CONFIG_OPTION_NETWORK_PING_SYSTEM";
			case CONFIG_OPTION_INLINE_RAY_TRACING:			return "CONFIG_OPTION_INLINE_RAY_TRACING";
			case CONFIG_OPTION_AA:							return "CONFIG_OPTION_AA";
			case CONFIG_OPTION_GLOSSY_REFLECTIONS:			return "CONFIG_OPTION_GLOSSY_REFLECTIONS";
			case CONFIG_OPTION_RAY_TRACED_SHADOWS:			return "CONFIG_OPTION_RAY_TRACED_SHADOWS";
			case CONFIG_OPTION_REFLECTIONS_SPP:				return "CONFIG_OPTION_REFLECTIONS_SPP";
			case CONFIG_OPTION_VOLUME_MUSIC:				return "CONFIG_OPTION_VOLUME_MUSIC";
			default:										return "CONFIG_OPTION_UNKNOWN";
		}
	}

	inline const EConfigOption StringToConfigOption(String str)
	{
		static const THashTable<String, EConfigOption> configMap = {
			{"CONFIG_OPTION_WINDOW_SIZE",				EConfigOption::CONFIG_OPTION_WINDOW_SIZE},
			{"CONFIG_OPTION_FULLSCREEN",				EConfigOption::CONFIG_OPTION_FULLSCREEN},
			{"CONFIG_OPTION_VOLUME_MASTER",				EConfigOption::CONFIG_OPTION_VOLUME_MASTER},
			{"CONFIG_OPTION_FIXED_TIMESTEMP",			EConfigOption::CONFIG_OPTION_FIXED_TIMESTEMP},
			{"CONFIG_OPTION_NETWORK_PORT",				EConfigOption::CONFIG_OPTION_NETWORK_PORT},
			{"CONFIG_OPTION_RAY_TRACING",				EConfigOption::CONFIG_OPTION_RAY_TRACING},
			{"CONFIG_OPTION_MESH_SHADER",				EConfigOption::CONFIG_OPTION_MESH_SHADER},
			{"CONFIG_OPTION_SHOW_RENDER_GRAPH",			EConfigOption::CONFIG_OPTION_SHOW_RENDER_GRAPH},
			{"CONFIG_OPTION_RENDER_GRAPH_NAME",			EConfigOption::CONFIG_OPTION_RENDER_GRAPH_NAME},
			{"CONFIG_OPTION_LINE_RENDERER",				EConfigOption::CONFIG_OPTION_LINE_RENDERER},
			{"CONFIG_OPTION_SHOW_DEMO",					EConfigOption::CONFIG_OPTION_SHOW_DEMO},
			{"CONFIG_OPTION_DEBUGGING",					EConfigOption::CONFIG_OPTION_DEBUGGING},
			{"CONFIG_OPTION_CAMERA_FOV",				EConfigOption::CONFIG_OPTION_CAMERA_FOV},
			{"CONFIG_OPTION_CAMERA_NEAR_PLANE",			EConfigOption::CONFIG_OPTION_CAMERA_NEAR_PLANE},
			{"CONFIG_OPTION_CAMERA_FAR_PLANE",			EConfigOption::CONFIG_OPTION_CAMERA_FAR_PLANE},
			{"CONFIG_OPTION_STREAM_PHYSX",				EConfigOption::CONFIG_OPTION_STREAM_PHYSX},
			{"CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG",	EConfigOption::CONFIG_OPTION_VALIDATION_LAYER_IN_DEBUG},
			{"CONFIG_OPTION_NETWORK_PROTOCOL",			EConfigOption::CONFIG_OPTION_NETWORK_PROTOCOL},
			{"CONFIG_OPTION_NETWORK_PING_SYSTEM",		EConfigOption::CONFIG_OPTION_NETWORK_PING_SYSTEM},
			{"CONFIG_OPTION_INLINE_RAY_TRACING",		EConfigOption::CONFIG_OPTION_INLINE_RAY_TRACING},
			{"CONFIG_OPTION_AA",						EConfigOption::CONFIG_OPTION_AA},
			{"CONFIG_OPTION_GLOSSY_REFLECTIONS",		EConfigOption::CONFIG_OPTION_GLOSSY_REFLECTIONS},
			{"CONFIG_OPTION_RAY_TRACED_SHADOWS",		EConfigOption::CONFIG_OPTION_RAY_TRACED_SHADOWS},
			{"CONFIG_OPTION_REFLECTIONS_SPP",			EConfigOption::CONFIG_OPTION_REFLECTIONS_SPP},
			{"CONFIG_OPTION_VOLUME_MUSIC",				EConfigOption::CONFIG_OPTION_VOLUME_MUSIC},
		};

		auto itr = configMap.find(str);
		if (itr != configMap.end()) {
			return itr->second;
		}
		return EConfigOption::CONFIG_OPTION_UNKNOWN;
	}
}