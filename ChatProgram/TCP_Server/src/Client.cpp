#include "Client.h"

void Client::AddRoomId(std::string roomId)
{
	if (!IsPresentInRoom(roomId))
	{
		roomIDs.push_back(roomId);
	}
}

void Client::RemoveId(std::string roomId)
{
	if (IsPresentInRoom(roomId))
	{
		roomIDs.erase(std::remove(roomIDs.begin(), roomIDs.end(), roomId), roomIDs.end());
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
