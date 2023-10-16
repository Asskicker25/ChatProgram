
// WinSock2 Windows Sockets
#define WIN32_LEAN_AND_MEAN

#include <GLFW/glfw3.h>
#include <iostream>
#include <conio.h>
#include <map>
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <Events.h>
#include <thread>
#include <Message.h>
#include <Buffer.h>
#include <conio.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <sstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8017"

enum WindowState
{
	EnterName,
	EnterRoomID,
	Chat,
};

struct ClientMessage
{
	std::string roomID;
	Message message;
};


void DrawImguiWindow(WindowState& windowState, ImVec4 clearColor, ImGuiIO io, int windowWidth, int windowHeight);
void GetKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

int screenWidth{ 800 };
int screenHeight{ 600 };
int currentRoomIndex = 0;
int prevMessagesCount = 0;

Events cleanupEvents;

SOCKET serverSocket;

std::string clientName;

std::vector<std::string> chatMessages;
std::vector<std::string> roomID;

std::queue<ClientMessage> clientMessages;

bool serverConnected = false;

std::condition_variable threadPausedCondition;
std::mutex threadPausedMtx;

std::atomic<bool> threadPaused = true;
std::mutex serverMtx;

static char chatText[512] = "";
static char roomText[512] = "";

struct addrinfo* info = nullptr;
struct addrinfo hints;

WindowState windowState  = WindowState::EnterName;

void FreeAddressInfo();
void CloseSocket();

void ConnectToServer();
void HandleRecvServer();
void HandleSendServer();
void AddRoomId(std::string newRoomID);
void RemoveRoomId(std::string removeRoomId);
void ChangeToNextRoomId(bool increase = true);
void AddMessageToGui(const std::string& message);
void AddMessageToQueue(const ClientMessage& clientMessage);
void AddChatToQueue(std::string message, std::string roomId);
void SendConnectedToRoom(bool connected);
std::string GetCurrentRoomID();
bool RoomIdExists(std::string roomId);


int main(void)
{
	GLFWwindow* window;

	if (!glfwInit())
		return -1;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(screenWidth, screenHeight, "Media Player", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, GetKeyboardCallback);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);


	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	////Getting Client name
	//std::cout << "Enter your name : ";
	//std::cin >> clientName;


	AddMessageToGui("\n");
	AddMessageToGui("************************************");
	AddMessageToGui("\n");

	

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{

		/* Poll for and process events */
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
		glViewport(0, 0, screenWidth, screenHeight);

		int imguiWindowWidth = screenWidth / 1.25f;
		int imguiWindowHeight = screenHeight / 1.25f;

		DrawImguiWindow(windowState, clear_color, io, imguiWindowWidth, imguiWindowHeight);

		/* Render here */
		ImGui::Render();

		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	cleanupEvents.Invoke();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void ConnectToServer()
{
	if (serverConnected) return;

	WSAData wsaData;
	int result, error;

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		AddMessageToGui("Winsock Initialization failed with error" + result);
		//std::cout << "Winsock initialization failed with error : " << result << std::endl;
		return;
	}
	AddMessageToGui("Winsock Initialized Successfully");
	//std::cout << "Winsock Initialized Successfully" << std::endl;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Getting AddressInfo
	result = getaddrinfo("10.0.0.3", DEFAULT_PORT, &hints, &info);
	if (result != 0)
	{
		AddMessageToGui("Getting Address failed with error : " + result);
		//std::cout << "Getting Address failed with error : " << result << std::endl;
		cleanupEvents.Addfunction("WSACleanup", WSACleanup);
		cleanupEvents.Invoke();
		return;
	}
	AddMessageToGui("Address fetched Successfully");

	//std::cout << "Address fetched Successfully" << std::endl;


	//Creating Socket
	serverSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (serverSocket == INVALID_SOCKET)
	{
		AddMessageToGui("Socket creation failed with error : " + WSAGetLastError());
		//std::cout << "Socket creation failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Free Address", FreeAddressInfo);
		cleanupEvents.Invoke();
		return;
	}

	AddMessageToGui("Address fetched Successfully");
	//std::cout << "Address fetched Successfully" << std::endl;



	//Connecting to Server
	result = connect(serverSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == INVALID_SOCKET)
	{
		AddMessageToGui("Connecting to Server failed with error : " + WSAGetLastError());
		//std::cout << "Connecting to Server failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Close Socket", CloseSocket);
		cleanupEvents.Invoke();
		return;
	}

	serverConnected = true;
	AddMessageToGui("Connected to Server Successfully");
	//std::cout << "Connected to Server Successfully" << std::endl;

	
	std::thread serverRecvThread([]()
		{
			HandleRecvServer();
		});

	serverRecvThread.detach();

	std::thread serverSendThread([]()
		{
			HandleSendServer();
		});

	serverSendThread.detach();
}

void DrawImguiWindow(WindowState& windowState, ImVec4 clearColor, ImGuiIO io, int windowWidth, int windowHeight)
{
	int spacingUnit = windowWidth * 0.005f;

	ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
	ImGui::SetNextWindowPos(ImVec2((screenWidth / 2) - windowWidth / 2, (screenHeight / 2) - windowHeight / 2));

	std::stringstream ss;
	ss << "TCP Client";

	if (clientName != "")
	{
		ss << " : " << clientName;
	}

	if (roomID.size() > 0)
	{
		ss << "   Rooms : ";

		for (int i = 0; i < roomID.size(); i++)
		{
			ss<<"[" << roomID[i] << "]";
		}
	}

	ImGui::Begin(ss.str().c_str());

	ImGui::SetWindowFontScale(windowWidth * 0.0025f);

	int windowWidthCell = windowWidth / 6;

	if (windowState == WindowState::EnterName)
	{
		ImGui::Spacing();
		ImGui::Dummy(ImVec2(0.0f, spacingUnit * 20));

		ImGui::Text("Enter Name : ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidthCell * 3);
		ImGui::InputText("##Name", chatText, IM_ARRAYSIZE(chatText));
		//ImGui::SetNextItemWidth(windowWidthCell * 1);
		ImGui::SameLine();
		if (ImGui::Button("Enter", ImVec2(0, (ImGui::GetTextLineHeight()) * 1.15)))
		{
			if (chatText[0] != '\0')
			{
				clientName = chatText;
				strncpy_s(chatText, sizeof(chatText), "", sizeof(chatText));
				windowState = WindowState::EnterRoomID;

				ImGui::End();
				return;
			}
			
		}
	}

	if (windowState == WindowState::EnterRoomID)
	{
		ImGui::Spacing();
		ImGui::Dummy(ImVec2(0.0f, spacingUnit * 20));

		ImGui::Text("Room ID : ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidthCell * 3);
		ImGui::InputText("##Room", roomText, IM_ARRAYSIZE(roomText));
		//ImGui::SetNextItemWidth(windowWidthCell * 1);
		ImGui::SameLine();
		if (ImGui::Button("Connect", ImVec2(0, (ImGui::GetTextLineHeight()) * 1.15)))
		{
			if (roomText[0] != '\0')
			{
				AddRoomId(roomText);

				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				ConnectToServer();
				SendConnectedToRoom(true);
				//strncpy_s(chatText, sizeof(chatText), "", sizeof(chatText));
				windowState = WindowState::Chat;

				ImGui::End();
				return;
			}
		}
	}

	if (windowState == WindowState::Chat)
	{
		ImGui::BeginChild("ChatMessages", ImVec2(0, windowHeight - (windowHeight / 4)), true);

		ImGui::SetScrollY(FLT_MAX);

		if (prevMessagesCount != chatMessages.size())
		{
			float totalHeight = ImGui::GetTextLineHeightWithSpacing();
			for (const std::string& message : chatMessages) {
				totalHeight += ImGui::GetTextLineHeightWithSpacing(); // Adjust this line height as needed
			}

			if (totalHeight > (windowHeight - (windowHeight / 4))) {
				ImGui::SetScrollY(totalHeight - (windowHeight - (windowHeight / 4)));
			}

			prevMessagesCount = chatMessages.size();
		}

		for (const std::string& message : chatMessages)
		{
			ImGui::TextWrapped(message.c_str());
		}
		
		ImGui::EndChild();

		ImGui::Spacing();
		ImGui::Dummy(ImVec2(0.0f, spacingUnit * 1));

		if (ImGui::Button(GetCurrentRoomID().c_str(), ImVec2(windowWidthCell * 0.75f,0.0f )))
		{
			ChangeToNextRoomId();
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(spacingUnit * 3, 0.0f));
		ImGui::SameLine();

		if (ImGui::Button("Join Room", ImVec2(0, 0)))
		{
			windowState = WindowState::EnterRoomID;
			ImGui::End();
			return;
		}

		ImGui::SameLine();

		if (ImGui::Button("Leave Room", ImVec2(0, 0)))
		{
			SendConnectedToRoom(false);
			RemoveRoomId(GetCurrentRoomID());
			ImGui::End();
			return;
		}

		if (!RoomIdExists(GetCurrentRoomID()))
		{
			ImGui::SameLine();
			ImGui::Text("Room Id doesn't exist");
		}


		ImGui::Text("Chat   : ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidthCell * 3);
		ImGui::InputText("##Chat", chatText, IM_ARRAYSIZE(chatText));
		ImGui::SetNextItemWidth(windowWidthCell * 1);
		ImGui::SameLine();
		if (ImGui::Button("SEND", ImVec2(0, (ImGui::GetTextLineHeight()) * 1.15)))
		{
			if (chatText[0] != '\0')
			{
				if (RoomIdExists(GetCurrentRoomID()))
				{
					std::string newStr(chatText);
					AddChatToQueue(newStr, GetCurrentRoomID());
					strncpy_s(chatText, sizeof(chatText), "", sizeof(chatText));
				}
				
			}
		}
	}


	//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::End();

}

void HandleRecvServer()
{
	//std::unique_lock<std::mutex> lock(serverMtx);
	while (serverConnected)
	{
		Buffer recvBuffer(4);

		int result, error;

		result = recv(serverSocket, recvBuffer.GetBufferData(), recvBuffer.GetBufferSize(), 0);
		if (result == SOCKET_ERROR)
		{
			error = WSAGetLastError();

			if (error == WSAECONNRESET || error == ECONNRESET)
			{
				AddMessageToGui("Lost Connection to Server");
				//std::cout << "Lost Connection to Server" << std::endl;

				//std::unique_lock<std::mutex> lock(serverMtx);
				serverConnected = false;

				cleanupEvents.Invoke();
			}
			else
			{
				AddMessageToGui("Receiving message from server failed with error : " + WSAGetLastError());
				//std::cout << "Receiving message from server failed with error : " << WSAGetLastError() << std::endl;
			}

		}
		else
		{
			uint32 packetSize = recvBuffer.ReadUInt32BE();
			Buffer messageBuffer(packetSize);

			result = recv(serverSocket, messageBuffer.GetBufferData(), messageBuffer.GetBufferSize(), 0);

			Message recvMessage = messageBuffer.ReadMessage();

			if (recvMessage.commandType == Message::CommandType::Chat)
			{
				AddMessageToGui(recvMessage.GetMessageDataString());
				//std::cout << recvMessage.GetMessageDataString() << std::endl;
			}
		}
	}

}

void HandleSendServer()
{
	//std::unique_lock<std::mutex> lock(serverMtx);
	while (serverConnected)
	{

		//Creating a buffer to send when connected
		int result, error;

		if (!clientMessages.empty())
		{
			//std::this_thread::sleep_for(std::chrono::milliseconds(100));

			ClientMessage clientMessage = clientMessages.front();
			clientMessages.pop();

			Buffer sendBuffer;

			std::string newStr(clientMessage.message.GetMessageDataString());

			sendBuffer.WriteMessage(clientMessage.message);

			//Sending client name when connected
			result = send(serverSocket, sendBuffer.GetBufferData(), sendBuffer.GetBufferSize(), 0);

			std::cout << "Result " << result << std::endl;
			std::cout << "PacketSize " << clientMessage.message.packetSize << std::endl;

			if (result == SOCKET_ERROR)
			{
				error = WSAGetLastError();

				if (error == WSAECONNRESET || error == ECONNRESET)
				{
					AddMessageToGui("Lost Connection to Server");
					//std::cout << "Lost Connection to Server" << std::endl;

					//std::unique_lock<std::mutex> lock(serverMtx);
					serverConnected = false;
					cleanupEvents.Invoke();
				}
				else
				{
					AddMessageToGui("Sending Message to Server failed with error : " + WSAGetLastError());
					//std::cout << "Sending Message to Server failed with error : " << WSAGetLastError() << std::endl;
				}
			}

			AddMessageToGui("[You] : " + clientMessage.message.GetMessageDataString());
		}
	}
}

void AddMessageToGui(const std::string& message)
{
	chatMessages.push_back(message);
}

void SendConnectedToRoom(bool connected)
{
	int result;

	//Creating a buffer to send when connected
	Buffer clientNameBuffer;

	Message clientNameMessage;
	clientNameMessage.commandType = connected ? Message::CommandType::JoinedRoom : Message::CommandType::LeftRoom;
	clientNameMessage.messageType = Message::Type::String;
	clientNameMessage.roomID = GetCurrentRoomID();
	clientNameMessage.SetMessageDataString(clientName);
	//std::string str = "Surya";
	//clientNameMessage.SetMessageDataString(str);
	//clientNameMessage.messageData = (uint32*)50;

	clientNameBuffer.WriteMessage(clientNameMessage);

	//Sending client name when connected
	result = send(serverSocket, clientNameBuffer.GetBufferData(), clientNameBuffer.GetBufferSize(), 0);
	if (result == SOCKET_ERROR)
	{
		AddMessageToGui("Sending Name to Server failed with error : " + WSAGetLastError());
		//std::cout << "Sending Name to Server failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Invoke();
	}

	//std::string newStr((char*)clientNameMessage1.messageData);
	//std::cout << "Client name sent to Server Successfully : " << clientNameMessage1.GetMessageDataString() << std::endl;
	AddMessageToGui("Connected to the [" + GetCurrentRoomID() + "] room");
	//std::cout << "Client name sent to Server Successfully" << std::endl << std::endl;
	//std::cout << "Message sent to Server Successfully : " << (uint32)clientNameMessage1.messageData << std::endl;
}

void FreeAddressInfo()
{
	freeaddrinfo(info);
}

void CloseSocket()
{
	closesocket(serverSocket);
}

void AddRoomId(std::string newRoomID)
{
	if (!RoomIdExists(newRoomID))
	{
		roomID.push_back(newRoomID);
		ChangeToNextRoomId();
	}
}

void RemoveRoomId(std::string remomveRoomId)
{
	if (RoomIdExists(remomveRoomId))
	{
		roomID.erase(std::remove(roomID.begin(), roomID.end(),remomveRoomId),roomID.end());
		if (roomID.size() == 0)
		{
			windowState = WindowState::EnterRoomID;
		}
		ChangeToNextRoomId(false);
	}
}

bool RoomIdExists(std::string roomId)
{
	bool roomExists = false;

	for (int i = 0; i < roomID.size(); i++)
	{
		if (roomID[i] == roomId)
		{
			roomExists = true;
			return roomExists;
		}
	}
	return roomExists;
}

void AddMessageToQueue(const ClientMessage& clientMessage)
{
	clientMessages.push(clientMessage);
}

void AddChatToQueue(std::string chatText, std::string roomId)
{
	ClientMessage clientMessage;

	Message message;
	message.commandType = Message::CommandType::Chat;
	message.messageType = Message::Type::String;
	message.SetRoomId(roomId);
	message.SetMessageDataString(chatText);

	clientMessage.message = message;

	//std::string newStr(clientMessage.message.GetMessageDataString());

	/*for (int i = 0; i < 50; i++)
	{
		AddMessageToQueue(clientMessage);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}*/

	AddMessageToQueue(clientMessage);


	std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

void ChangeToNextRoomId(bool increase)
{
	if(increase)
		currentRoomIndex++;

	if (currentRoomIndex >= roomID.size())
	{
		currentRoomIndex = 0;
	}
}

std::string GetCurrentRoomID()
{
	return roomID[currentRoomIndex];
}

void GetKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_SPACE)
		{
			//mediaPlayer.TogglePauseAudio();
			//mediaPlayer.AdjustPitch(1.0f);
		}
	}
}