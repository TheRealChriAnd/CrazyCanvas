#include "Engine/EngineConfig.h"

#include "Log/Log.h"

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

#include <fstream>

namespace LambdaEngine
{
	rapidjson::Document EngineConfig::s_ConfigDocument = {};
	String EngineConfig::s_FilePath;
	using namespace rapidjson;

	bool EngineConfig::LoadFromFile(const argh::parser& flagParser)
	{
		/*	Production-released products will likely not be started with any command line arguments,
			that is when the production config is used */
		constexpr const char* pDefaultPathPostfix = "production";
		String pathPostfix;
		flagParser({ "--state" }, pDefaultPathPostfix) >> pathPostfix;
		s_FilePath = "engine_config_" + String(pathPostfix) + ".json";

		FILE* pFile = fopen(s_FilePath.c_str(), "r");
		if (!pFile)
		{
			// The state does not have its own config file, try the default config path
			s_FilePath = "engine_config_" + String(pDefaultPathPostfix) + ".json";

			pFile = fopen(s_FilePath.c_str(), "r");
			if (!pFile)
			{
				// TODO: We should probably create a default so that the user does not need to have a config file to even run the application
				LOG_WARNING("Engine config could not be opened: %s", s_FilePath.c_str());
				DEBUGBREAK();
				return false;
			}
		}

		char readBuffer[2048];
		FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		s_ConfigDocument.ParseStream(inputStream);

		return fclose(pFile) == 0;
	}

	bool EngineConfig::WriteToFile()
	{
		FILE* pFile = fopen(s_FilePath.c_str(), "w");
		if (!pFile)
		{
			LOG_WARNING("Engine config could not be opened: %s", s_FilePath.c_str());
			return false;
		}

		char writeBuffer[2048];
		FileWriteStream outputStream(pFile, writeBuffer, sizeof(writeBuffer));

		PrettyWriter<FileWriteStream> writer(outputStream);
		s_ConfigDocument.Accept(writer);

		return fclose(pFile) == 0;
	}

	bool EngineConfig::GetBoolProperty(const String& propertyName)
	{
		return s_ConfigDocument[propertyName.c_str()].GetBool();
	}

	float EngineConfig::GetFloatProperty(const String& propertyName)
	{
		return s_ConfigDocument[propertyName.c_str()].GetFloat();
	}

	int EngineConfig::GetIntProperty(const String& propertyName)
	{
		return s_ConfigDocument[propertyName.c_str()].GetInt();
	}

	double EngineConfig::GetDoubleProperty(const String& propertyName)
	{
		return s_ConfigDocument[propertyName.c_str()].GetDouble();
	}

	String EngineConfig::GetStringProperty(const String& propertyName)
	{
		return s_ConfigDocument[propertyName.c_str()].GetString();
	}

	TArray<float> EngineConfig::GetFloatArrayProperty(const String& propertyName)
	{
		const Value& arr = s_ConfigDocument[propertyName.c_str()];
		TArray<float> tArr;
		for (auto& itr : arr.GetArray())
			tArr.PushBack(itr.GetFloat());

		return tArr;
	}

	TArray<int> EngineConfig::GetIntArrayProperty(const String& propertyName)
	{
		const Value& arr = s_ConfigDocument[propertyName.c_str()];
		TArray<int> tArr;
		for (auto& itr : arr.GetArray())
			tArr.PushBack(itr.GetInt());

		return tArr;
	}

	bool EngineConfig::SetBoolProperty(const String& propertyName, const bool value)
	{
		if (s_ConfigDocument.HasMember(propertyName.c_str()))
		{
			s_ConfigDocument[propertyName.c_str()].SetBool(value);

			return true;
		}

		return false;
	}

	bool EngineConfig::SetFloatProperty(const String& propertyName, const float value)
	{
		if (s_ConfigDocument.HasMember(propertyName.c_str()))
		{
			s_ConfigDocument[propertyName.c_str()].SetFloat(value);

			return true;
		}
		return false;
	}

	bool EngineConfig::SetIntProperty(const String& propertyName, const int value)
	{
		if (s_ConfigDocument.HasMember(propertyName.c_str()))
		{
			s_ConfigDocument[propertyName.c_str()].SetFloat(value);

			return true;
		}

		return false;
	}

	bool EngineConfig::SetDoubleProperty(const String& propertyName, const double value)
	{
		if (s_ConfigDocument.HasMember(propertyName.c_str()))
		{
			s_ConfigDocument[propertyName.c_str()].SetDouble(value);

			return true;
		}
		return false;
	}

	bool EngineConfig::SetStringProperty(const String& propertyName, const String& string)
	{
		if (s_ConfigDocument.HasMember(propertyName.c_str()))
		{
			s_ConfigDocument[propertyName.c_str()].SetString(string.c_str(), static_cast<SizeType>(strlen(string.c_str())), s_ConfigDocument.GetAllocator());

			return true;
		}
		return false;
	}

	bool EngineConfig::SetFloatArrayProperty(const String& propertyName, const TArray<float>& arr)
	{
		if (s_ConfigDocument.HasMember(propertyName.c_str()))
		{
			Document::AllocatorType& allocator = s_ConfigDocument.GetAllocator();

			auto& newArr = s_ConfigDocument[propertyName.c_str()].SetArray();
			newArr.Reserve(arr.GetSize(), allocator);

			for (auto& itr : arr)
				newArr.PushBack(Value().SetFloat(itr), allocator);

			StringBuffer strBuf;
			PrettyWriter<StringBuffer> writer(strBuf);
			s_ConfigDocument.Accept(writer);

			return true;
		}

		return false;
	}

	bool EngineConfig::SetIntArrayProperty(const String& propertyName, const TArray<int>& arr)
	{
		if (s_ConfigDocument.HasMember(propertyName.c_str()))
		{
			Document::AllocatorType& allocator = s_ConfigDocument.GetAllocator();

			auto& newArr = s_ConfigDocument[propertyName.c_str()].SetArray();
			newArr.Reserve(arr.GetSize(), allocator);

			for (auto& itr : arr)
				newArr.PushBack(Value().SetInt(itr), allocator);

			StringBuffer strBuf;
			PrettyWriter<StringBuffer> writer(strBuf);
			s_ConfigDocument.Accept(writer);

			return true;
		}

		return false;
	}
}
