#include "Game/GameConsole.h"

#include "Input/API/Input.h"

#include "Rendering/ImGuiRenderer.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Engine/EngineConfig.h"

#include <regex>
#include <imgui.h>

namespace LambdaEngine
{
	bool GameConsole::Init()
	{
		if (!InitCommands())
		{
			return false;
		}

		return EventQueue::RegisterEventHandler(this, &GameConsole::OnKeyPressed);
	}

	bool GameConsole::InitCommands()
	{
		// Help
		{
			ConsoleCommand cmdHelp;
			cmdHelp.Init("help", false);
			cmdHelp.AddFlag("d", Arg::EType::EMPTY);
			cmdHelp.AddDescription("Shows all commands descriptions.", { {"d", "Used to show debug commands also."} });
			BindCommand(cmdHelp, [this](CallbackInput& input)->void
				{
					for (auto i : m_CommandMap)
					{
						ConsoleCommand& cCmd = i.second.first;
						if (!cCmd.IsDebug() || (input.Flags.find("d") != input.Flags.end()))
						{
							std::string type = cCmd.IsDebug() ? " [DEBUG]" : "";
							ConsoleCommand::Description desc = cCmd.GetDescription();
							PushInfo(i.first + type + ":");
							PushMsg("\t" + desc.MainDesc, glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
							for (auto flagDesc : desc.FlagDescs)
								PushMsg("\t\t-" + flagDesc.first + ": " + flagDesc.second, glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
						}
					}
				});
		}

		// Clear
		{
			ConsoleCommand cmdClear;
			cmdClear.Init("clear", false);
			cmdClear.AddFlag("h", Arg::EType::EMPTY);
			cmdClear.AddDescription("Clears the visible text in the console.", { {"h", "Clears the history."} });
			BindCommand(cmdClear, [this](CallbackInput& input)->void
				{
					UNREFERENCED_VARIABLE(input);
					m_Items.Clear();
				});
		}

		// Exit
		{
			ConsoleCommand cmdExit;
			cmdExit.Init("exit", false);
			cmdExit.AddDescription("Terminate the application");
			BindCommand(cmdExit, [this](CallbackInput& input)->void
				{
					UNREFERENCED_VARIABLE(input);
					CommonApplication::Get()->Terminate();
				});
		}

		// Enable input
		{
			ConsoleCommand cmdInput;
			cmdInput.Init("enable_input", false);
			cmdInput.AddArg(Arg::EType::BOOL);
			cmdInput.AddDescription("Enable or disable application input");
			BindCommand(cmdInput, [this](CallbackInput& input)->void
				{
					if (input.Arguments[0].Value.Boolean)
					{
						Input::Enable();
					}
					else
					{
						Input::Disable();
					}
				});
		}

		// Main window width
		{
			ConsoleCommand cmdWidth;
			cmdWidth.Init("window_width", false);
			cmdWidth.AddArg(Arg::EType::INT);
			cmdWidth.AddDescription("Set main window width");
			BindCommand(cmdWidth, [this](CallbackInput& input)->void
				{
					TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
					if (mainWindow)
					{
						const uint16 height = mainWindow->GetHeight();
						mainWindow->SetSize((uint16)input.Arguments.GetFront().Value.Int32, (uint16)height);
					}
				});
		}

		// Main window height
		{
			ConsoleCommand cmdHeight;
			cmdHeight.Init("window_height", false);
			cmdHeight.AddArg(Arg::EType::INT);
			cmdHeight.AddDescription("Set main window height");
			BindCommand(cmdHeight, [this](CallbackInput& input)->void
				{
					TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
					if (mainWindow)
					{
						const uint16 width = mainWindow->GetWidth();
						mainWindow->SetSize(width, (uint16)input.Arguments.GetFront().Value.Int32);
					}
				});
		}

		// Main window maximize
		{
			ConsoleCommand cmdMaximize;
			cmdMaximize.Init("window_maximize", false);
			BindCommand(cmdMaximize, [this](CallbackInput& input)->void
				{
					UNREFERENCED_VARIABLE(input);
					TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
					if (mainWindow)
					{
						mainWindow->Maximize();
					}
				});
		}

		// Main window minimize
		{
			ConsoleCommand cmdMinimize;
			cmdMinimize.Init("window_minimize", false);
			BindCommand(cmdMinimize, [this](CallbackInput& input)->void
				{
					UNREFERENCED_VARIABLE(input);
					TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
					if (mainWindow)
					{
						mainWindow->Minimize();
					}
				});
		}

		// Main window fullscreen
		{
			ConsoleCommand cmdFullscreen;
			cmdFullscreen.Init("window_toggle_fullscreen", false);
			cmdFullscreen.AddDescription("Set main window to fullscreen");
			BindCommand(cmdFullscreen, [this](CallbackInput& input)->void
				{
					UNREFERENCED_VARIABLE(input);
					TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
					if (mainWindow)
					{
						mainWindow->ToggleFullscreen();
					}
				});
		}

		// Main window size
		{
			ConsoleCommand cmdSize;
			cmdSize.Init("window_size", false);
			cmdSize.AddFlag("w", Arg::EType::INT);
			cmdSize.AddFlag("h", Arg::EType::INT);
			cmdSize.AddDescription("Set main window size", { { "w", "Height of window" }, { "h", "Height of window" }, });
			BindCommand(cmdSize, [this](CallbackInput& input)->void
			{
				TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
				if (mainWindow)
				{
					uint16 width = mainWindow->GetWidth();
					auto widthPair = input.Flags.find("w");
					if (widthPair != input.Flags.end())
					{
						width = (uint16)widthPair->second.Arg.Value.Int32;
					}

					uint16 height = mainWindow->GetWidth();
					auto heightPair = input.Flags.find("h");
					if (heightPair != input.Flags.end())
					{
						height = (uint16)heightPair->second.Arg.Value.Int32;
					}

					mainWindow->SetSize(width, height);
				}
			});
		}

		// Test Command
		{
			ConsoleCommand cmd;
			cmd.Init("clo", true);
			cmd.AddArg(Arg::EType::STRING);
			cmd.AddFlag("l", Arg::EType::INT);
			cmd.AddFlag("i", Arg::EType::EMPTY);
			cmd.AddDescription("Does blah and do bar.");

			GameConsole::Get().BindCommand(cmd, [](GameConsole::CallbackInput& input)->void
			{
				std::string s1 = input.Arguments.GetFront().Value.String;
				std::string s2 = input.Flags.find("i") == input.Flags.end() ? "no set" : "set";
				std::string s3 = "no set";
				auto it = input.Flags.find("l");
				if (it != input.Flags.end())
					s3 = "set with a value of " + std::to_string(it->second.Arg.Value.Int32);
				LOG_INFO("Command Called with argument '%s' and flag i was %s and flag l was %s.", s1.c_str(), s2.c_str(), s3.c_str());
			});
		}

		// Set Boolean in config
		{
			ConsoleCommand cmdSetBool;
			cmdSetBool.Init("set_bool", true);
			cmdSetBool.AddArg(Arg::EType::STRING);
			cmdSetBool.AddArg(Arg::EType::BOOL);
			cmdSetBool.AddDescription("Change boolean value in config file with a given key\n\t 'set_bool key bool'");
			BindCommand(cmdSetBool, [this](GameConsole::CallbackInput& input)->void
				{
					if (!EngineConfig::SetBoolProperty(StringToConfigOption(input.Arguments.GetFront().Value.String), input.Arguments.GetBack().Value.Boolean))
						PushError((input.Arguments.GetFront().Value.String));
				});
		}

		// Set Float in config
		{
			ConsoleCommand cmdSetFloat;
			cmdSetFloat.Init("set_float", true);
			cmdSetFloat.AddArg(Arg::EType::STRING);
			cmdSetFloat.AddArg(Arg::EType::FLOAT);
			cmdSetFloat.AddDescription("Change float value in config file with a given key\n\t 'set_float key value'");
			BindCommand(cmdSetFloat, [this](GameConsole::CallbackInput& input)->void
				{
					if (!EngineConfig::SetFloatProperty(StringToConfigOption(input.Arguments.GetFront().Value.String), input.Arguments.GetBack().Value.Float32))
						PushError((input.Arguments.GetFront().Value.String));
				});
		}

		// Set Integer in config
		{
			ConsoleCommand cmdSetInt;
			cmdSetInt.Init("set_int", true);
			cmdSetInt.AddArg(Arg::EType::STRING);
			cmdSetInt.AddArg(Arg::EType::INT);
			cmdSetInt.AddDescription("Change integer value in config file with a given key\n\t 'set_int key value'");
			BindCommand(cmdSetInt, [this](GameConsole::CallbackInput& input)->void
				{
					if (!EngineConfig::SetIntProperty(StringToConfigOption(input.Arguments.GetFront().Value.String), input.Arguments.GetBack().Value.Int32))
						PushError((input.Arguments.GetFront().Value.String));
				});
		}

		// Set Double in config
		{
			ConsoleCommand cmdSetDouble;
			cmdSetDouble.Init("set_double", true);
			cmdSetDouble.AddArg(Arg::EType::STRING);
			cmdSetDouble.AddArg(Arg::EType::FLOAT);
			cmdSetDouble.AddDescription("Change double value in config file with a given key\n\t 'set_double key value'");
			BindCommand(cmdSetDouble, [this](GameConsole::CallbackInput& input)->void
				{
					if (!EngineConfig::SetDoubleProperty(StringToConfigOption(input.Arguments.GetFront().Value.String), static_cast<double>(input.Arguments.GetBack().Value.Float32)))
						PushError((input.Arguments.GetFront().Value.String));
				});
		}


		// Set String in config
		{
			ConsoleCommand cmdSetString;
			cmdSetString.Init("set_string", true);
			cmdSetString.AddArg(Arg::EType::STRING);
			cmdSetString.AddArg(Arg::EType::STRING);
			cmdSetString.AddDescription("Change boolean value in config file with a given key\n\t 'set_string key string'");
			BindCommand(cmdSetString, [this](GameConsole::CallbackInput& input)->void
				{
					if (!EngineConfig::SetBoolProperty(StringToConfigOption(input.Arguments.GetFront().Value.String), input.Arguments.GetBack().Value.String))
						PushError((input.Arguments.GetFront().Value.String));
				});
		}

		// Set Float Array in config
		{
			ConsoleCommand cmdSetFloatArray;
			cmdSetFloatArray.Init("set_float_array", true);
			cmdSetFloatArray.AddArg(Arg::EType::STRING);
			cmdSetFloatArray.AddArg(Arg::EType::STRING);
			cmdSetFloatArray.AddDescription("Change array values in settings with a given key\n\t 'set_array key 1.0,1.0'\n\t OBS: The decimal point needs to be in the argument");
			BindCommand(cmdSetFloatArray, [this](GameConsole::CallbackInput& input)->void
				{
					TArray<float> arr;
					String arg = input.Arguments.GetBack().Value.String;

					while (arg.length() > 0)
					{
						if (std::regex_match(arg.substr(0, arg.find(",")), std::regex("(-[0-9]*\\.[0-9]+)|(-[0-9]+\\.[0-9]*)")))
						{
							arr.PushBack(std::stof(arg.substr(0, arg.find(","))));
						}
						else if (std::regex_match(arg.substr(0, arg.find(",")), std::regex("([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)")))
						{
							arr.PushBack(std::stof(arg.substr(0, arg.find(","))));
						}
						else
						{
							PushError(arg.substr(0, arg.find(",")));
						}

						if (std::string::npos != arg.find(","))
							arg.erase(0, arg.find(",") + std::string(",").length());
						else
							arg.erase(0, arg.find(","));
					}

					if (!EngineConfig::SetFloatArrayProperty(StringToConfigOption(input.Arguments.GetFront().Value.String), arr))
						PushError((input.Arguments.GetFront().Value.String));
				});
		}

		// Set Array in config
		{
			ConsoleCommand cmdSetIntArray;
			cmdSetIntArray.Init("set_int_array", true);
			cmdSetIntArray.AddArg(Arg::EType::STRING);
			cmdSetIntArray.AddArg(Arg::EType::STRING);
			cmdSetIntArray.AddDescription("Change array values in settings with a given key\n\t 'set_array key 1.0,1.0'\n\t OBS: The decimal point needs to be in the argument");
			BindCommand(cmdSetIntArray, [this](GameConsole::CallbackInput& input)->void
				{
					TArray<int> arr;
					String arg = input.Arguments.GetBack().Value.String;

					while (arg.length() > 0)
					{
						if (std::regex_match(arg.substr(0, arg.find(",")), std::regex("-[0-9]+")))
						{
							arr.PushBack(std::stoi(arg.substr(0, arg.find(","))));
						}
						else if (std::regex_match(arg.substr(0, arg.find(",")), std::regex("[0-9]+")))
						{
							arr.PushBack(std::stoi(arg.substr(0, arg.find(","))));
						}
						else
						{
							PushError(arg.substr(0, arg.find(",")));
						}

						if (std::string::npos != arg.find(","))
							arg.erase(0, arg.find(",") + std::string(",").length());
						else
							arg.erase(0, arg.find(","));
					}

					if (!EngineConfig::SetIntArrayProperty(StringToConfigOption(input.Arguments.GetFront().Value.String), arr))
						PushError((input.Arguments.GetFront().Value.String));
				});
		}


		// Save config to file
		{
			ConsoleCommand cmdSaveSettings;
			cmdSaveSettings.Init("save_settings", true);
			cmdSaveSettings.AddDescription("Save changes into config file");
			BindCommand(cmdSaveSettings, [](CallbackInput& input)->void
				{
					UNREFERENCED_VARIABLE(input);
					EngineConfig::WriteToFile();
				});
		}

		return true;
	}

	bool GameConsole::Release()
	{
		static bool s_IsReleased = false;
		GameConsole& console = GameConsole::Get();
		if (!s_IsReleased)
		{
			s_IsReleased = true;
			return EventQueue::UnregisterEventHandler(&console, &GameConsole::OnKeyPressed);
		}

		return false;
	}

	void GameConsole::Tick()
	{
		// Do not draw if not active.
		if (!m_IsActive)
		{
			return;
		}

		ImVec2 popupPos;
		ImVec2 popupSize;

		ImGuiRenderer::Get().DrawUI([&]()
		{
			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoTitleBar;

			TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
			uint32 width = mainWindow->GetWidth();
			uint32 height = mainWindow->GetHeight();
			const uint32 standardHeight = 200;

			// Draw a console window at the top right of the viewport.
			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver); // Standard position
			ImGui::SetNextWindowSize(ImVec2((float)width, (float)standardHeight), ImGuiCond_FirstUseEver); // Standard size
			ImGui::SetNextWindowSizeConstraints(ImVec2((float)width, 70), ImVec2((float)width, (float)height)); // Window constraints

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f); // Make more transparent
			ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0); // Remove grip, resize works anyway
			ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0); // Remove grip, resize works anyway
			ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0); // Remove grip, resize works anyway

			if (ImGui::Begin("Console", (bool*)0, flags))
			{
				bool hasFocus = false;

				// History
				const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
				ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f); // Make less transparent
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

				// Only display visible text to see history.
				ImGuiListClipper clipper(m_Items.GetSize());
				while (clipper.Step())
				{
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						Item& item = m_Items[i];
						const char* str = item.Str.c_str();
						ImVec4 color = ImVec4(item.Color.r, item.Color.g, item.Color.b, item.Color.a);
						ImGui::PushStyleColor(ImGuiCol_Text, color);
						ImGui::TextUnformatted(str);
						ImGui::PopStyleColor();
					}
				}

				if (m_ScrollToBottom || (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
					ImGui::SetScrollHereY(0.0f);

				m_ScrollToBottom = false;

				ImGui::PopStyleVar();
				ImGui::PopStyleVar();

				ImGui::EndChild();
				ImGui::Separator();

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f); // Make less transparent

				// Command line
				// static char s_Buf[256];

				ImGui::PushItemWidth((float)width);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.9f));

				const ImGuiInputTextFlags input_text_flags = 
					ImGuiInputTextFlags_EnterReturnsTrue | 
					ImGuiInputTextFlags_CallbackCompletion | 
					ImGuiInputTextFlags_CallbackHistory | 
					ImGuiInputTextFlags_CallbackEdit | 
					ImGuiInputTextFlags_CallbackAlways;
				
				if (ImGui::InputText("###Input", m_Buf, 256, input_text_flags,
					[](ImGuiInputTextCallbackData* data)->int {
						GameConsole* console = (GameConsole*)data->UserData;
						return console->TextEditCallback(data);
					}, (void*)this))
				{
					if (m_ActivePopupIndex != -1)
					{
						strcpy(m_Buf, m_PopupSelectedText.c_str());
						m_ActivePopupIndex = -1;
						m_Candidates.Clear();
						m_UpdatePositionCursor = true;
					}
					else
					{
						if (m_Buf[0])
						{
							std::string buff = std::string(m_Buf);
							ExecCommand(buff);
						}

						strcpy(m_Buf, "");
					}

					hasFocus = true;
				}

				ImGui::PopStyleColor();
				ImGui::PopItemWidth();
				ImGui::PopStyleVar();

				if (hasFocus)
				{
					ImGui::SetItemDefaultFocus();
					ImGui::SetKeyboardFocusHere(-1); // Set focus to the text field.
				}

				if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
				{
					ImGui::SetKeyboardFocusHere(-1);
				}

				popupSize = ImVec2(ImGui::GetItemRectSize().x - 60, ImGui::GetTextLineHeightWithSpacing() * 2);
				popupPos = ImGui::GetItemRectMin();
				popupPos.y += ImGui::GetItemRectSize().y;
			}
			ImGui::End();

			// Draw popup autocomplete window
			if (m_Candidates.GetSize() > 0)
			{
				bool isActiveIndex = false;
				bool popupOpen = true;
				ImGuiWindowFlags popupFlags =
					ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_HorizontalScrollbar |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing;

				if (m_Candidates.GetSize() < 10)
				{
					popupFlags |= ImGuiWindowFlags_NoScrollbar;
					popupSize.y = (ImGui::GetTextLineHeight() + 8) * m_Candidates.GetSize();
				}
				else
				{
					popupSize.y = (ImGui::GetTextLineHeight() + 8) * 10;
				}

				ImGui::SetNextWindowPos(popupPos);
				ImGui::SetNextWindowSize(popupSize);
				ImGui::Begin("candidates_popup", &popupOpen, popupFlags);
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
				ImGui::PushAllowKeyboardFocus(false);

				for (int i = 0; i < (int)m_Candidates.GetSize(); i++)
				{
					isActiveIndex = m_ActivePopupIndex == i;
					ImGui::PushID(i);
					if (ImGui::Selectable(m_Candidates[i].c_str(), &isActiveIndex))
					{
						strcpy(m_Buf, m_Candidates[i].c_str());
						m_PopupSelectedText = m_Candidates[i];
						m_ActivePopupIndex = -1;
						m_Candidates.Clear();
						m_UpdatePositionCursor = true;
						ImGui::PopID();
						break;
					}
					ImGui::PopID();

					if (isActiveIndex && m_PopupSelectionChanged)
					{
						ImGui::SetScrollHere();
						m_PopupSelectedText = m_Candidates[i];
						m_PopupSelectionChanged = false;
					}
				}

				ImGui::PopAllowKeyboardFocus();
				ImGui::PopStyleVar();
				ImGui::End();
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		});
	}

	void GameConsole::BindCommand(ConsoleCommand cmd, std::function<void(CallbackInput&)> callback)
	{
		m_CommandMap[cmd.GetName()] = std::pair<ConsoleCommand, std::function<void(CallbackInput&)>>(cmd, callback);
	}

	void GameConsole::PushError(const std::string& msg)
	{
		PushMsg("Error:" + msg, glm::vec4(1.f, 0.f, 0.f, 1.f));
	}

	void GameConsole::PushInfo(const std::string& msg)
	{
		PushMsg(msg, glm::vec4(1.f, 1.f, 0.f, 1.f));
	}

	void GameConsole::PushMsg(const std::string& line, glm::vec4 color)
	{
		Item item = {};
		item.Str = line;
		item.Color = color;
		m_Items.PushBack(item);
		m_ScrollToBottom = true;
	}

	GameConsole& GameConsole::Get()
	{
		static GameConsole console;
		return console;
	}

	GameConsole::GameConsole()
	{
	}

	GameConsole::~GameConsole()
	{
		GameConsole::Get().Release();
	}

	int GameConsole::ExecCommand(const std::string& data)
	{
		std::string command = data;
		m_History.PushBack(command);

		Item item = {};
		item.Str = data;
		item.Color = glm::vec4(1.f, 1.f, 1.f, 1.f);
		m_Items.PushBack(item);

		m_ScrollToBottom = true;

		size_t cmdPos = command.find(" ");
		size_t pos = cmdPos != std::string::npos ? cmdPos : command.length();
		std::string token = command.substr(0, pos);
		command.erase(0, pos + std::string(" ").length());

		auto it = m_CommandMap.find(token);
		if (it == m_CommandMap.end())
		{
			PushError("Command '" + token + "' not found.");
			return 0;
		}

		ConsoleCommand& cmd = it->second.first;
#ifndef LAMBDA_DEVELOPMENT
		if (cmd.IsDebug())
		{
			PushError("Command '" + token + "' is debug only.");
			return 0;
		}
#endif
		Flag* preFlag = nullptr;
		std::unordered_map<std::string, Flag> flags;

		bool wasFlag = false;
		uint32 flagArgIndex = 0;
		uint32 argIndex = 0;
		while (((pos = command.find(" ")) != std::string::npos) || ((pos = command.length()) > 0))
		{
			Arg arg;
			token = command.substr(0, pos);
			if (std::regex_match(token, std::regex("-[[:alpha:]].*")))
			{
				wasFlag = true;
				flagArgIndex = 0;
				std::string name = token.substr(1);
				auto flagIt = cmd.GetFlags().find(name);
				if (flagIt == cmd.GetFlags().end())
				{
					PushError("'" + token + "' is an invalid flag!");
					return 0;
				}

				flags[name] = flagIt->second;
				preFlag = &flags[name];
			}
			else
			{
				FillArg(arg, token);
				if (wasFlag)
				{
					if (cmd.GetFlags()[preFlag->Name].Arg.Type != Arg::EType::EMPTY)
					{
						// Check if it is an array after the flag.
						if (preFlag->Args.GetSize() > 0)
						{
							// An array of arguments is used.
							preFlag->Args[flagArgIndex++] = arg;
							preFlag->NumUsedArgs++;
							if (flagArgIndex >= preFlag->Args.GetSize())
							{
								flagArgIndex = 0;
								wasFlag = false;
							}
						}
						else // One argument is used.
						{
							preFlag->NumUsedArgs = 1;
							preFlag->Arg = arg;
							wasFlag = false;
						}
					}
					else
					{
						bool res = AddArg(argIndex, arg, cmd);
						if (!res) return 0;
						argIndex++;
						wasFlag = false;
					}
				}
				else
				{
					bool res = AddArg(argIndex, arg, cmd);
					if (!res) return 0;
					argIndex++;
				}
			}

			if (std::string::npos != command.find(" "))
				command.erase(0, pos + std::string(" ").length());
			else
				command.erase(0, pos);
		}

		uint32 size = cmd.GetArguments().GetSize();
		if (argIndex != size) // Error too few arguments!
		{
			PushError("Too few arguments!");
			return 0;
		}

		// Call function
		CallbackInput input;
		input.Arguments = cmd.GetArguments();
		input.Flags = flags;
		it->second.second(input);

		return 0;
	}

	void GameConsole::FillArg(Arg& arg, std::string token)
	{
		arg.Type = Arg::EType::STRING;
		strcpy(arg.Value.String, token.c_str());

		if (std::regex_match(token, std::regex("-[0-9]+")))
		{
			arg.Type = Arg::EType::INT;
			arg.Value.Int32 = std::stoi(token);
		}
		else if (std::regex_match(token, std::regex("[0-9]+")))
		{
			arg.Type = Arg::EType::INT;
			arg.Value.Int32 = std::stoi(token);
		}
		else if (std::regex_match(token, std::regex("(-[0-9]*\\.[0-9]+)|(-[0-9]+\\.[0-9]*)")))
		{
			arg.Type = Arg::EType::FLOAT;
			arg.Value.Float32 = std::stof(token);
		}
		else if (std::regex_match(token, std::regex("([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)")))
		{
			arg.Type = Arg::EType::FLOAT;
			arg.Value.Float32 = std::stof(token);
		}
		std::for_each(token.begin(), token.end(), [](char& c) { c = (char)std::tolower(c); });
		if (std::regex_match(token, std::regex("(false)|(true)")))
		{
			arg.Type = Arg::EType::BOOL;
			arg.Value.Boolean = token == "false" ? false : true;
		}
	}

	bool GameConsole::AddArg(uint32 index, Arg arg, ConsoleCommand& cmd)
	{
		uint32 size = cmd.GetArguments().GetSize();
		if (index >= size) // Error too many arguments
		{
			PushError("Too many arguments!");
			return false;
		}

		// Add special case for booleans, this makes it possible to write an integer as argument for a bool
		if (cmd.GetArguments()[index].Type == Arg::EType::BOOL)
		{
			if (arg.Type != Arg::EType::BOOL)
			{
				if (arg.Type == Arg::EType::INT)
				{
					int32 value = arg.Value.Int32;
					arg.Type = Arg::EType::BOOL;
					arg.Value.Boolean = (value != 0);
				}
				else
				{
					PushError("Wrong argument type!");
					return false;
				}
			}
		}
		else if (cmd.GetArguments()[index].Type != arg.Type) // Error wrong type
		{
			PushError("Wrong argument type!");
			return false;
		}

		cmd.GetArguments()[index].Value = arg.Value;
		return true;
	}

	bool GameConsole::OnKeyPressed(const KeyPressedEvent& event)
	{
		if (event.Key == EKey::KEY_GRAVE_ACCENT && !event.IsRepeat)
		{
			m_IsActive = !m_IsActive;
			if (m_IsActive)
				Input::PushInputLayer(EInputLayer::DEBUG);
			else
				Input::PopInputLayer();
			return true;
		}

		return false;
	}

	int GameConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
	{
		if (m_UpdatePositionCursor)
		{
			m_UpdatePositionCursor = false;
			data->CursorPos = (int)m_PopupSelectedText.length();
			data->InsertChars(data->CursorPos, " ");
			m_PopupSelectedText = "";
		}

		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackEdit:
		{
			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf)
			{
				const char c = word_start[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
					break;
				word_start--;
			}

			m_Candidates.Clear();
			for (auto& cmd : m_CommandMap)
			{
				const char* command = cmd.first.c_str();
				int32 d = -1;
				int32 n = (int32)(word_end - word_start);
				const char* s1 = command;
				const char* s2 = word_start;
				while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
				{
					s1++;
					s2++;
					n--;
				}
				if (d == 0)
				{
					m_Candidates.PushBack(command);
				}
			}

			break;
		}
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf)
			{
				const char c = word_start[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
					break;
				word_start--;
			}

			// Build a list of candidates
			TArray<const char*> candidates;
			//m_Candidates.Clear();
			for (auto& cmd : m_CommandMap)
			{
				const char* command = cmd.first.c_str();
				int32 d = 0;
				int32 n = (int32)(word_end - word_start);
				const char* s1 = command;
				const char* s2 = word_start;
				while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
				{
					s1++;
					s2++;
					n--;
				}
				if (d == 0)
				{
					candidates.PushBack(command);
				}
			}

			if (candidates.GetSize() == 0)
			{
				// No match
				m_ActivePopupIndex = -1;
			}
			else if (candidates.GetSize() == 1)
			{
				// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0]);
				data->InsertChars(data->CursorPos, " ");
			}
			else if (m_ActivePopupIndex != -1)
			{
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, m_PopupSelectedText.c_str());
				data->InsertChars(data->CursorPos, " ");
				m_ActivePopupIndex = -1;
				m_PopupSelectedText = "";
				m_Candidates.Clear();
			}
			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
			if (m_Candidates.GetSize() == 0)
			{
				// Show history when nothing is typed (no candidates)
				const int prevHistoryIndex = m_HistoryIndex;
				if (data->EventKey == ImGuiKey_UpArrow)
				{
					if (m_HistoryIndex == -1)
						m_HistoryIndex = m_History.GetSize() - 1;
					else if (m_HistoryIndex > 0)
						m_HistoryIndex--;
				}
				else if (data->EventKey == ImGuiKey_DownArrow)
				{
					if (m_HistoryIndex != -1)
						if (++m_HistoryIndex >= (int)m_History.GetSize())
							m_HistoryIndex = -1;
				}

				if (prevHistoryIndex != m_HistoryIndex)
				{
					const char* historyStr = (m_HistoryIndex >= 0) ? m_History[m_HistoryIndex].c_str() : "";
					data->DeleteChars(0, data->BufTextLen);
					data->InsertChars(0, historyStr);
				}
			}
			else
			{
				// Navigate candidates list
				if (data->EventKey == ImGuiKey_UpArrow)
				{
					if (m_ActivePopupIndex <= 0)
					{
						m_ActivePopupIndex = ((int32)(m_Candidates.GetSize()) - 1);
						m_PopupSelectionChanged = true;
					}
					else
					{
						m_ActivePopupIndex--;
						m_PopupSelectionChanged = true;
					}
				}
				else if (data->EventKey == ImGuiKey_DownArrow)
				{
					if (m_ActivePopupIndex >= ((int32)(m_Candidates.GetSize()) - 1))
					{
						m_ActivePopupIndex = 0;
						m_PopupSelectionChanged = true;
					}
					else
					{
						m_ActivePopupIndex++;
						m_PopupSelectionChanged = true;
					}
				}
			}
			break;
		}
		}
		return 0;
	}
}
