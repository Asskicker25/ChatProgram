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

Events cleanupEvents;

std::vector<SOCKET> clientList;

SOCKET listenSocket;
SOCKET newClientSocket;

struct addrinfo* info = nullptr;
struct addrinfo hints;

void FreeAddressInfo();
void CloseSocket();
void AddNewClientSocket();


int main(int arg, char** argv)
{

	WSAData wsaData;
	int result;

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

	std::thread addNewClients(AddNewClientSocket);

	while (true)
	{
	}

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

void AddNewClientSocket()
{
	newClientSocket = accept(listenSocket, NULL, NULL);

	if (newClientSocket == INVALID_SOCKET)
	{
		cleanupEvents.Invoke();
	}

	std::cout << "New Client Added" << std::endl;
}