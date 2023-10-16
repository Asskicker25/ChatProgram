#pragma once

#include "Client.h"

#include <string>
#include <vector>
#include <WinSock2.h>

struct Client
{
	std::string clientName;
	std::vector<std::string> roomIDs;
	SOCKET clientSocket;
	bool terminateThread;

	void AddRoomId(std::string roomId);
	void RemoveId(std::string roomId);
	bool IsPresentInRoom(std::string roomID);
};