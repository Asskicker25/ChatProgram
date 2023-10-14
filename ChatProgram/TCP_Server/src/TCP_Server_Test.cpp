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
#include<Message.h>
#include<Buffer.h>
#include <conio.h>
#include <mutex>
#include <condition_variable>
#include <queue>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8017"

//Create Address Info
//Socket
//Bind Socket
//Listen
//Accept - block call //Client connection to server
//Recv from client socket
//Send message to client socket
//Cleanup

struct Client
{
	std::string clientName;
	SOCKET clientSocket;
	bool terminateThread;
};

struct ClientMessage
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
std::queue<ClientMessage> clientMessages; 
//std::vector<ClientMessage> clientJoinedMessages; 

SOCKET listenSocket;

bool serverInitialized = false;

struct addrinfo* info = nullptr;
struct addrinfo hints;

void FreeAddressInfo();
void CloseSocket();
void AddNewClient();
void HandleRecvClient(Client* client);
void HandleSendClient();
void AddMessageToQueue(const ClientMessage& clientMessage);


int main(int argc, char** argv)
{

	WSAData wsaData;
	int result;

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (result != 0)
	{
		std::cout << "Winsock initialization failed with error : " << result << std::endl;
		return -1;
	}

	serverInitialized = true;
	std::cout << "Winsock Initialized Successfully" << std::endl;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &info);
	if (result != 0)
	{
		std::cout << "Getting Address failed with error : " << result << std::endl;
		cleanupEvents.Addfunction("WSACleanup", WSACleanup);
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Address fetched Successfully" << std::endl;


	listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Free Address", FreeAddressInfo);
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Socket created Successfully" << std::endl;

	result = bind(listenSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == SOCKET_ERROR)
	{
		std::cout << "Binding socked failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Close Socket", CloseSocket);
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Binding socket successful " << std::endl;


	result = listen(listenSocket, SOMAXCONN);
	if (result == SOCKET_ERROR)
	{
		std::cout << "Listening socked failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Listening socket successfull " << std::endl;

	std::cout << std::endl;
	std::cout << "************************************" << std::endl;
	std::cout << std::endl;

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

	while (true)
	{
	}

	cleanupEvents.Invoke();

	return 0;
}

void FreeAddressInfo()
{
	freeaddrinfo(info);
}

void CloseSocket()
{
	closesocket(listenSocket);
}

void AddMessageToQueue(const ClientMessage& clientMessage)
{
	std::unique_lock<std::mutex> lock(messageQueueMutex);
	clientMessages.push(clientMessage);
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

			std::unique_lock<std::mutex> lock(clientListMtx);

			clientList.push_back(newClient);
			clientThreads.push_back(std::move(newClientThread));
		}
	}

	std::cout << "Thread Closed" << std::endl;
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
				printf("%s has disconnected from the room\n", client->clientName.c_str());

				client->terminateThread = true;
				closesocket(client->clientSocket);

				std::unique_lock<std::mutex> lock(clientListMtx);
				clientList.erase(std::remove(clientList.begin(), clientList.end(), client), clientList.end());
			}
			else
			{
				std::cout << "Receiving message from Client failed with error : " << WSAGetLastError() << std::endl;
			}
		}
		else
		{
			Message message = clientBuffer.ReadMessage();
			ClientMessage clientMessage;

			if (message.commandType == Message::CommandType::SetName)
			{
				client->clientName = message.GetMessageDataString();

				Message sendMessage;

				sendMessage.commandType = Message::CommandType::Chat;
				sendMessage.messageType = Message::Type::String;

				std::string newStr(client->clientName + " has connect to the room");

				sendMessage.SetMessageDataString(newStr);

				//std::cout << "Added To Queue : " << sendMessage.GetMessageDataString() << std::endl;

				clientMessage.message = sendMessage;
				clientMessage.client = client;

				AddMessageToQueue(clientMessage);

				printf("%s has connected to the room\n", client->clientName.c_str());
			}
			else if (message.commandType == Message::CommandType::Chat)
			{
				std::string newStr("[" + client->clientName + "] : " + message.GetMessageDataString());

				Message sendMessage;

				sendMessage.commandType = Message::CommandType::Chat;
				sendMessage.messageType = Message::Type::String;

				sendMessage.SetMessageDataString(newStr);

				clientMessage.message = sendMessage;
				clientMessage.client = client;

				AddMessageToQueue(clientMessage);
				std::cout << newStr  << std::endl;
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
		std::unique_lock<std::mutex> lock(messageQueueMutex);
		if(!clientMessages.empty())
		{
			ClientMessage message = clientMessages.front();
			clientMessages.pop();

			Buffer sendBuffer;
			
			sendBuffer.WriteMessage(message.message);

			std::unique_lock<std::mutex> lock(clientListMtx);

			for (int i = 0; i < clientList.size(); i++)
			{
				if (clientList[i]->clientName == message.client->clientName)
				{
					continue;
				}

				result = send(clientList[i]->clientSocket, sendBuffer.GetBufferData(), sendBuffer.GetBufferSize(), 0);
				if (result == SOCKET_ERROR)
				{
					std::cout << "Sending message to Client failed with error : " << WSAGetLastError() << std::endl;
				}
			}

			
		}
	}
}


