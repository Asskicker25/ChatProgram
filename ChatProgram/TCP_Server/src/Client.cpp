#include "Client.h"

void Client::AddRoomId(std::string roomId)
{
	if (!IsPresentInRoom(roomId))
	{
		roomIDs.push_back(roomId);
	}
}

bool Client::IsPresentInRoom(std::string roomID)
{
	bool presentInRoom = false;

	for (int i = 0; i < roomIDs.size(); i++)
	{
		if (roomIDs[i] == roomID)
		{
			presentInRoom = true;
			return presentInRoom;
		}
	}
	return presentInRoom;
}
