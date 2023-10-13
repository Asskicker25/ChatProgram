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

//GetAddress
//Socket
//Connect
//Recv
//Send

Events cleanupEvents;

SOCKET serverSocket;

struct addrinfo* info = nullptr;
struct addrinfo hints;

void FreeAddressInfo();
void CloseSocket();

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

	std::cout << "Winsock Initialized Successfully" << std::endl;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo("localHost", DEFAULT_PORT, &hints, &info);
	if (result != 0)
	{
		std::cout << "Getting Address failed with error : " << result << std::endl;
		cleanupEvents.Addfunction("WSACleanup", WSACleanup);
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Address fetched Successfully" << std::endl;



	serverSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (serverSocket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Free Address", FreeAddressInfo);
		cleanupEvents.Invoke();
		return -1;
	}

	std::cout << "Socket created Successfully" << std::endl;


	result = connect(serverSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == INVALID_SOCKET)
	{
		std::cout << "Connecting to Server failed with error : " << WSAGetLastError() << std::endl;
		cleanupEvents.Addfunction("Close Socket", CloseSocket);
		cleanupEvents.Invoke();
		return -1;
	}
	std::cout << "Connecting to Server Successfully : " << std::endl;


	while (true)
	{
	}

	system("Pause");

	cleanupEvents.Invoke();

	return 0;
}

void FreeAddressInfo()
{
	freeaddrinfo(info);
}

void CloseSocket()
{
	closesocket(serverSocket);
}