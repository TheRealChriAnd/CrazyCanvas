#include "Sandbox.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Network/API/PlatformSocketFactory.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Audio/AudioSystem.h"

Sandbox::Sandbox() : 
	m_pResourceManager(nullptr)
{
	using namespace LambdaEngine;
    
	//TCP TEST
	/*ISocketTCP* server = PlatformSocketFactory::CreateSocketTCP();
	server->Bind("127.0.0.1", 4444);
	server->Listen();

	ISocketTCP* client = PlatformSocketFactory::CreateSocketTCP();
	client->Connect("127.0.0.1", 4444);

	ISocketTCP* serverClient = server->Accept();

	std::string data = "Hello Guy!";
	uint32 bytesSent;
	serverClient->Send(data.c_str(), data.length(), bytesSent);

	char buffer[256];
	uint32 bytesReceived;
	client->Receive(buffer, 256, bytesReceived);

	LOG_MESSAGE(buffer);
	serverClient->Close();
	client->Close();
	server->Close();


	//UDP TEST
	ISocketUDP* socket1 = PlatformSocketFactory::CreateSocketUDP();
	ISocketUDP* socket2 = PlatformSocketFactory::CreateSocketUDP();

	socket2->Bind("127.0.0.1", 4444);
	socket1->SendTo(data.c_str(), data.length(), bytesSent, "127.0.0.1", 4444);

	buffer[256];
	std::string sender;
	uint16 port;
	socket2->ReceiveFrom(buffer, 256, bytesReceived, sender, port);
	LOG_MESSAGE(buffer);
	LOG_MESSAGE(sender.c_str());
	LOG_MESSAGE("%d", port);

	data = "Vafan Guy!";
	socket2->SendTo(data.c_str(), data.length(), bytesSent, sender, port);
	socket1->ReceiveFrom(buffer, 256, bytesReceived, sender, port);
	LOG_MESSAGE(buffer);
	LOG_MESSAGE(sender.c_str());
	LOG_MESSAGE("%d", port);
	socket1->Close();
	socket2->Close();


	//UDP Broadcast TEST
	LOG_MESSAGE("Broadcast");
	socket1 = PlatformSocketFactory::CreateSocketUDP();
	socket2 = PlatformSocketFactory::CreateSocketUDP();

	socket1->EnableBroadcast();
	socket2->EnableBroadcast();

	socket1->Bind("", 4444);

	data = "Ny data Guy!";

	socket2->Broadcast(data.c_str(), data.length(), bytesSent, 4444);

	socket1->ReceiveFrom(buffer, 256, bytesReceived, sender, port);
	LOG_MESSAGE(buffer);
	LOG_MESSAGE(sender.c_str());
	LOG_MESSAGE("%d", port);*/

	m_pResourceManager = new LambdaEngine::ResourceManager(LambdaEngine::RenderSystem::GetDevice(), LambdaEngine::AudioSystem::GetDevice());
	m_TestSound = m_pResourceManager->LoadSoundFromFile("../Assets/Sounds/smb_gameover.wav", ESoundFlags::LOOPING);
}

Sandbox::~Sandbox()
{
	SAFEDELETE(m_pResourceManager);
}

void Sandbox::TestResourceManager()
{
	LOG_MESSAGE("\n-------Resource Handler Testing Start-------");

	

	////Audio Test
	//{
	//	GUID_Lambda failedSoundGUID = pResourceHandler->LoadSoundFromFile("THIS/SHOULD/FAIL.obj");
	//	GUID_Lambda successSoundGUID = pResourceHandler->LoadSoundFromFile("../Assets/Sounds/smb_gameover.wav");

	//	Sound* pSound = pResourceHandler->GetSound(successSoundGUID);
	//	pSound->Play();
	//}

	////Scene Test
	//{
	//	std::vector<GraphicsObject> sponzaGraphicsObjects;
	//	pResourceHandler->LoadSceneFromFile("../Assets/Scenes/sponza/", "sponza.obj", sponzaGraphicsObjects);
	//}

	////Mesh Test
	//{
	//	GUID_Lambda failedMeshGUID = pResourceHandler->LoadMeshFromFile("THIS/SHOULD/FAIL.obj");
	//	GUID_Lambda sucessMeshGUID = pResourceHandler->LoadMeshFromFile("../Assets/Meshes/bunny.obj");

	//	Mesh* pBunnyMesh = ResourceLoader::LoadMeshFromFile(pGraphicsDevice, "../Assets/Meshes/bunny.obj");

	//	SAFEDELETE(pBunnyMesh);
	//	SAFEDELETE(pResourceHandler);
	//}

	LOG_MESSAGE("-------Resource Handler Testing End-------\n");
}

void Sandbox::OnKeyDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Pressed: %d", key);

	using namespace LambdaEngine;

	Sound* pTestSound = m_pResourceManager->GetSound(m_TestSound);

	if (key == EKey::KEY_KP_5)
	{
		pTestSound->Toggle();
	}
	else if (key == EKey::KEY_KP_8)
	{
		pTestSound->SetVolume(pTestSound->GetVolume() + 0.05f);
	}
	else if (key == EKey::KEY_KP_2)
	{
		pTestSound->SetVolume(pTestSound->GetVolume() - 0.05f);
	}
	else if (key == EKey::KEY_KP_9)
	{
		pTestSound->SetPitch(pTestSound->GetPitch() + 0.05f);
	}
	else if (key == EKey::KEY_KP_7)
	{
		pTestSound->SetPitch(pTestSound->GetPitch() - 0.05f);
	}
	else if (key == EKey::KEY_KP_3)
	{
		pTestSound->SetPanning(pTestSound->GetPanning() + 0.05f);
	}
	else if (key == EKey::KEY_KP_1)
	{
		pTestSound->SetPanning(pTestSound->GetPanning() - 0.05f);
	}
}

void Sandbox::OnKeyHeldDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Held Down: %d", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Released: %d", key);
}

void Sandbox::OnMouseMove(int32 x, int32 y)
{
	LOG_MESSAGE("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Released: %d", button);
}

void Sandbox::OnScroll(int32 delta)
{
	LOG_MESSAGE("Mouse Scrolled: %d", delta);
}

void Sandbox::Tick()
{
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
        Sandbox* pSandbox = new Sandbox();
        Input::AddKeyboardHandler(pSandbox);
        Input::AddMouseHandler(pSandbox);
        
        return pSandbox;
    }
}
