
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

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8017"



void DrawImguiWindow(bool window, ImVec4 clearColor, ImGuiIO io, int windowWidth, int windowHeight);
void GetKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

int screenWidth{ 800 };
int screenHeight{ 600 };

Events cleanupEvents;

SOCKET serverSocket;

std::string clientName;

std::vector<std::string> chatMessages;

bool serverConnected = false;

std::condition_variable threadPausedCondition;
std::mutex threadPausedMtx;

std::atomic<bool> threadPaused = true;
std::mutex serverMtx;

static char chatText[512] = "";
static char roomText[512] = "";

struct addrinfo* info = nullptr;
struct addrinfo hints;

void FreeAddressInfo();
void CloseSocket();

void HandleRecvServer();
void HandleSendServer();
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
	int result, error;

	//Getting Client name
	std::cout << "Enter your name : ";
	std::cin >> clientName;

	//std::cout << std::endl;
	std::cout << "************************************" << std::endl;
	//std::cout << std::endl;
	AddMessageToGui("\n");
	AddMessageToGui("************************************");
	AddMessageToGui("\n");

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		AddMessageToGui("Winsock Initialization failed with error" + result);
		//std::cout << "Winsock initialization failed with error : " << result << std::endl;
		return -1;
	}
	AddMessageToGui("Winsock Initialized Successfully");
	//std::cout << "Winsock Initialized Successfully" << std::endl;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Getting AddressInfo
	result = getaddrinfo("localHost", DEFAULT_PORT, &hints, &info);
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


	//Creating Socket
	serverSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (serverSocket == INVALID_SOCKET)
	{
		AddMessageToGui("Socket creation failed with error : " + WSAGetLastError());
		//std::cout << "Socket creation failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Free Address", FreeAddressInfo);
		cleanupEvents.Invoke();
		return -1;
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
		return -1;
	}

	serverConnected = true;
	AddMessageToGui("Connected to Server Successfully");
	//std::cout << "Connected to Server Successfully" << std::endl;

	//Creating a buffer to send when connected
	Buffer clientNameBuffer;

	Message clientNameMessage;
	clientNameMessage.commandType = Message::CommandType::SetName;
	clientNameMessage.messageType = Message::Type::String;
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
	AddMessageToGui("Client name sent to Server Successfully");
	//std::cout << "Client name sent to Server Successfully" << std::endl << std::endl;
	//std::cout << "Message sent to Server Successfully : " << (uint32)clientNameMessage1.messageData << std::endl;

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

		int imguiWindowWidth = screenWidth / 2;
		int imguiWindowHeight = screenHeight / 2;

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


	ImGui::Begin("TCP Client");

	ImGui::SetWindowFontScale(windowWidth * 0.0025f);

	ImGui::BeginChild("ChatMessages", ImVec2(0, windowHeight - (windowHeight / 4)), true);
	// Loop through chat messages and display them here
	for (const std::string& message : chatMessages) {

		//ImGui::TextUnformatted(message.c_str());
		ImGui::TextWrapped(message.c_str());
	}
	ImGui::EndChild();

	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0.0f, spacingUnit * 1));

	int windowWidthCell = windowWidth / 6;
	ImGui::Text("RoomID : ");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(windowWidthCell * 1.5);
	ImGui::InputText("##Room", roomText, IM_ARRAYSIZE(roomText));

	ImGui::Text("Chat   : ");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(windowWidthCell * 3);
	ImGui::InputText("##Chat", chatText, IM_ARRAYSIZE(chatText));
	ImGui::SetNextItemWidth(windowWidthCell * 1);
	ImGui::SameLine();
	if (ImGui::Button("SEND", ImVec2(0, ImGui::GetTextLineHeight())))
	{
		// Handle the send action here
		std::lock_guard<std::mutex> lock(threadPausedMtx);
		threadPaused = false;
	}
	//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::End();
}


void HandleRecvServer()
{
	//std::unique_lock<std::mutex> lock(serverMtx);
	while (serverConnected)
	{
		Buffer recvBuffer(512);

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
			Message recvMessage = recvBuffer.ReadMessage();

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
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		//Creating a buffer to send when connected
		int result, error;

		std::string chat;

		AddMessageToGui("Enter Chat :");
		//std::cout << "Enter Chat : \n";
		if (threadPaused)
		{
			std::cout << "Thread Paused" << std::endl;
			// Wait for a signal to continue
			std::unique_lock<std::mutex> lock(threadPausedMtx);
			threadPausedCondition.wait(lock, [] { return !threadPaused; });
		}
		std::cout << "Thread UnPaused" << std::endl;

		AddMessageToGui(chatText);

		Buffer sendBuffer;

		Message sendMessage;
		sendMessage.commandType = Message::CommandType::Chat;
		sendMessage.messageType = Message::Type::String;
		sendMessage.SetMessageDataString(chat);

		sendBuffer.WriteMessage(sendMessage);

		//Sending client name when connected
		result = send(serverSocket, sendBuffer.GetBufferData(), sendBuffer.GetBufferSize(), 0);
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
	}
}

void AddMessageToGui(const std::string& message)
{
	chatMessages.push_back(message);
}

void FreeAddressInfo()
{
	freeaddrinfo(info);
}

void CloseSocket()
{
	closesocket(serverSocket);
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