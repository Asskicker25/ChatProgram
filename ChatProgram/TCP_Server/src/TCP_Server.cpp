
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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Client.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8017"


void DrawImguiWindow(bool window, ImVec4 clearColor, ImGuiIO io, int windowWidth, int windowHeight);
void GetKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

int screenWidth{ 800 };
int screenHeight{ 600 };



struct ServerMessage
{
	Client* client;
	Message message;
};


Events cleanupEvents;

std::mutex clientListMtx;
std::mutex messageQueueMutex;

std::condition_variable condition;


std::vector<Client*> clientList;
std::vector <std::thread> clientThreads;
//std::vector<std::string> roomIds;
std::queue<ServerMessage> serverMessages;
//std::vector<ClientMessage> clientJoinedMessages; 

std::vector<std::string> chatMessages;


SOCKET listenSocket;

bool serverInitialized = false;

struct addrinfo* info = nullptr;
struct addrinfo hints;

void FreeAddressInfo();
void CloseSocket();
void AddNewClient();
void HandleRecvClient(Client* client);
void HandleSendClient();
//void AddRoomID(std::string roomId);
//bool RoomIdExists(std::string roomId);
void AddMessageToQueue(const ServerMessage& serverMessages);

void AddMessageToGui(const std::string& message);



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
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	WSAData wsaData;
	int result;

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (result != 0)
	{
		AddMessageToGui("Winsock initialization failed with error :" + result);
		//std::cout << "Winsock initialization failed with error : " << result << std::endl;
		return -1;
	}

	serverInitialized = true;
	AddMessageToGui("Winsock Initialized Successfully");
	//std::cout << "Winsock Initialized Successfully" << std::endl;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &info);
	if (result != 0)
	{
		AddMessageToGui("Getting Address failed with error : " + result);
		//std::cout << "Getting Address failed with error : " << result << std::endl;
		cleanupEvents.Addfunction("WSACleanup", WSACleanup);
		cleanupEvents.Invoke();
		return -1;
	}

	AddMessageToGui("Address fetched Successfully");
	//std::cout << "Address fetched Successfully" << std::endl;


	listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		AddMessageToGui("Socket creation failed with error : " + WSAGetLastError());
		//std::cout << "Socket creation failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Free Address", FreeAddressInfo);
		cleanupEvents.Invoke();
		return -1;
	}

	AddMessageToGui("Socket created Successfully");
	//std::cout << "Socket created Successfully" << std::endl;

	result = bind(listenSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == SOCKET_ERROR)
	{
		AddMessageToGui("Binding socked failed with error : " + WSAGetLastError());
		//std::cout << "Binding socked failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Close Socket", CloseSocket);
		cleanupEvents.Invoke();
		return -1;
	}

	AddMessageToGui("Binding socket successful ");
	//std::cout << "Binding socket successful " << std::endl;


	result = listen(listenSocket, SOMAXCONN);
	if (result == SOCKET_ERROR)
	{
		AddMessageToGui("Listening socked failed with error : " + WSAGetLastError());
		//std::cout << "Listening socked failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Invoke();
		return -1;
	}

	AddMessageToGui("Listening socket successfull ");
	//std::cout << "Listening socket successfull " << std::endl;

	AddMessageToGui("\n ");
	AddMessageToGui("************************************ ");
	AddMessageToGui("\n ");


	/*std::cout << std::endl;
	std::cout << "************************************" << std::endl;
	std::cout << std::endl;*/

	std::thread addClientThread([]()
		{
			AddNewClient();
		});

	addClientThread.detach();

	std::thread sendMessagesToClient([]()
		{
			HandleSendClient();
		});

	sendMessagesToClient.detach();

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

		DrawImguiWindow(show_another_window, clear_color, io, imguiWindowWidth, imguiWindowHeight);

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



void DrawImguiWindow(bool window, ImVec4 clearColor, ImGuiIO io, int windowWidth, int windowHeight)
{
	int spacingUnit = windowWidth * 0.005f;

	ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
	ImGui::SetNextWindowPos(ImVec2((screenWidth / 2) - windowWidth / 2, (screenHeight / 2) - windowHeight / 2));


	ImGui::Begin("TCP Server");                         

	ImGui::SetWindowFontScale(windowWidth * 0.0025f);

	ImGui::BeginChild("ChatMessages", ImVec2(0, windowHeight - (windowHeight/5)), true);
	// Loop through chat messages and display them here
	for (const std::string& message : chatMessages) {
		//ImGui::TextUnformatted(message.c_str());
		ImGui::TextWrapped(message.c_str());
	}
	ImGui::EndChild();


	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0.0f, spacingUnit * 10));


	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::End();
}


void AddMessageToQueue(const ServerMessage& serverMessage)
{
	//std::unique_lock<std::mutex> lock(messageQueueMutex);
	serverMessages.push(serverMessage);
}

void AddNewClient()
{
	while (true)
	{
		SOCKET newClientSocket = accept(listenSocket, NULL, NULL);

		if (newClientSocket != INVALID_SOCKET)
		{
			Client* newClient = new Client();
			newClient->clientSocket = newClientSocket;

			std::thread newClientThread([newClient]() {
				HandleRecvClient(newClient);
				});

			newClientThread.detach();

			//std::unique_lock<std::mutex> lock(clientListMtx);

			clientList.push_back(newClient);
			clientThreads.push_back(std::move(newClientThread));
		}
	}

	AddMessageToGui("Add Client Thread Closed");
	//std::cout << "Thread Closed" << std::endl;
	condition.notify_all();
}

void HandleRecvClient(Client* client)
{
	int result, error;

	while (!client->terminateThread)
	{
		Buffer clientBuffer(512);

		//recv
		result = recv(client->clientSocket, clientBuffer.GetBufferData(), clientBuffer.GetBufferSize(), 0);
		if (result == SOCKET_ERROR)
		{
			error = WSAGetLastError();

			if (error == WSAECONNRESET || error == ECONNRESET)
			{

				AddMessageToGui(client->clientName + " has disconnected from the room");

				//printf("%s has disconnected from the room\n", client->clientName.c_str());

				client->terminateThread = true;
				closesocket(client->clientSocket);

				//std::unique_lock<std::mutex> lock(clientListMtx);
				clientList.erase(std::remove(clientList.begin(), clientList.end(), client), clientList.end());
			}
			else
			{
				AddMessageToGui("Receiving message from Client failed with error : " + WSAGetLastError());
				//std::cout << "Receiving message from Client failed with error : " << WSAGetLastError() << std::endl;
			}
		}
		else
		{
			Message message = clientBuffer.ReadMessage();
			ServerMessage serverMessage;

			if (message.commandType == Message::CommandType::SetName)
			{
				client->clientName = message.GetMessageDataString();

				//AddRoomID(message.GetRoomId());
				client->AddRoomId(message.GetRoomId());

				Message sendMessage;

				sendMessage.commandType = Message::CommandType::Chat;
				sendMessage.messageType = Message::Type::String;
				sendMessage.SetRoomId(message.GetRoomId());
				std::string newStr(client->clientName + " has connect to the room");

				sendMessage.SetMessageDataString(newStr);

				//std::cout << "Added To Queue : " << sendMessage.GetMessageDataString() << std::endl;

				serverMessage.message = sendMessage;
				serverMessage.client = client;

				AddMessageToQueue(serverMessage);

				//std::string addString(client->clientName);
				AddMessageToGui(client->clientName + " has connected to the room");
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				//printf("%s has connected to the room\n", client->clientName.c_str());
			}
			else if (message.commandType == Message::CommandType::Chat)
			{
				std::string newStr("[" + message.GetRoomId() + "]" + "[" + client->clientName + "] : " +
									message.GetMessageDataString());

				Message sendMessage;

				sendMessage.commandType = Message::CommandType::Chat;
				sendMessage.messageType = Message::Type::String;
				sendMessage.SetRoomId(message.roomID);

				sendMessage.SetMessageDataString(newStr);

				serverMessage.message = sendMessage;
				serverMessage.client = client;

				AddMessageToQueue(serverMessage);

				AddMessageToGui(newStr);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				//std::cout << "\n ";
			}
			//system("Pause");
		}
	}

	delete client;

}

void HandleSendClient()
{
	int result, error;

	while (true)
	{
		//std::unique_lock<std::mutex> lock(messageQueueMutex);
		if (!serverMessages.empty())
		{
			ServerMessage message = serverMessages.front();
			serverMessages.pop();

			Buffer sendBuffer;

			sendBuffer.WriteMessage(message.message);

			//std::unique_lock<std::mutex> lock(clientListMtx);

			for (int i = 0; i < clientList.size(); i++)
			{
				if (!clientList[i]->IsPresentInRoom(message.message.roomID) || clientList[i]->clientName == message.client->clientName)
				{
					continue;
				}

				result = send(clientList[i]->clientSocket, sendBuffer.GetBufferData(), sendBuffer.GetBufferSize(), 0);
				if (result == SOCKET_ERROR)
				{
					//AddMessageToGui("Sending message to Client failed with error : " + WSAGetLastError());
					std::cout << "Sending message to Client failed with error : " << WSAGetLastError() << std::endl;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

void AddMessageToGui(const std::string& message)
{
	chatMessages.push_back(message);
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
void FreeAddressInfo()
{
	freeaddrinfo(info);
}

void CloseSocket()
{
	closesocket(listenSocket);
}

//void AddRoomID(std::string roomId)
//{
//	if (!RoomIdExists(roomId))
//	{
//		roomIds.push_back(roomId);
//	}
//}
//
//bool RoomIdExists(std::string roomId)
//{
//	bool roomExists = false;
//
//	for (int i = 0; i < roomIds.size(); i++)
//	{
//		if (roomIds[i] == roomId)
//		{
//			roomExists = true;
//			return roomExists;
//		}
//	}
//	return false;
//}
//
//
