#pragma once

// WinSock2 Windows Sockets
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <Events.h>
#include <thread>
#include <Buffer.h>
#include <Message.h>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8017"

//GetAddress
//Socket
//Connect
//Recv
//Send

Events cleanupEvents;

SOCKET serverSocket;

std::string clientName;

bool serverConnected = false;

std::mutex serverMtx;

struct addrinfo* info = nullptr;
struct addrinfo hints;

void FreeAddressInfo();
void CloseSocket();

void HandleRecvServer();
void HandleSendServer();

int main(int argc, char** argv)
{
	WSAData wsaData;
	int result, error;

	//Getting Client name
	std::cout << "Enter your name : ";
	std::cin >> clientName;

	std::cout << std::endl;
	std::cout << "************************************" << std::endl;
	std::cout << std::endl;

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		std::cout << "Winsock initialization failed with error : " << result << std::endl;
		return -1;
	}

	std::cout << "Winsock Initialized Successfully" << std::endl;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Getting AddressInfo
	result = getaddrinfo("localHost", DEFAULT_PORT, &hints, &info);
	if (result != 0)
	{
		std::cout << "Getting Address failed with error : " << result << std::endl;
		cleanupEvents.Addfunction("WSACleanup", WSACleanup);
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Address fetched Successfully" << std::endl;


	//Creating Socket
	serverSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (serverSocket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Free Address", FreeAddressInfo);
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Socket created Successfully" << std::endl;



	//Connecting to Server
	result = connect(serverSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == INVALID_SOCKET)
	{
		std::cout << "Connecting to Server failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Close Socket", CloseSocket);
		cleanupEvents.Invoke();
		return -1;
	}

	serverConnected = true;
	std::cout << "Connected to Server Successfully" << std::endl;

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
		std::cout << "Sending Name to Server failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Invoke();
	}

	//std::string newStr((char*)clientNameMessage1.messageData);
	//std::cout << "Client name sent to Server Successfully : " << clientNameMessage1.GetMessageDataString() << std::endl;
	std::cout << "Client name sent to Server Successfully" << std::endl << std::endl;
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

	while (true)
	{

		//system("Pause");
	}

	//system("Pause");

	cleanupEvents.Invoke();

	return 0;
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
				std::cout << "Lost Connection to Server" << std::endl;

				std::unique_lock<std::mutex> lock(serverMtx);
				serverConnected = false;

				cleanupEvents.Invoke();
			}
			else
			{
				std::cout << "Receiving message from server failed with error : " << WSAGetLastError() << std::endl;
			}

		}
		else
		{
			Message recvMessage = recvBuffer.ReadMessage();

			if (recvMessage.commandType == Message::CommandType::Chat)
			{
				std::cout << recvMessage.GetMessageDataString() << std::endl;
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

		std::string chat;

		std::cout << "Enter Chat : \n";
		std::cin >> chat;

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
				std::cout << "Lost Connection to Server" << std::endl;

				//std::unique_lock<std::mutex> lock(serverMtx);
				serverConnected = false;
				cleanupEvents.Invoke();
			}
			else
			{
				std::cout << "Sending Message to Server failed with error : " << WSAGetLastError() << std::endl;
			}

			
		}
	}
}

void FreeAddressInfo()
{
	freeaddrinfo(info);
}

void CloseSocket()
{
	closesocket(serverSocket);
}