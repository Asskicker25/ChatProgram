#include "Message.h"

void Message::SetMessageDataUInt(const uint32& value)
{
	messageData = (uint32*)value;
}

void Message::SetMessageDataUShort(const ushort16& value)
{
	messageData = (ushort16*)value;
}

void Message::SetMessageDataString(const std::string& value)
{
	messageData = (char*)value.c_str();
}

void Message::SetRoomId(const std::string& roomId)
{
	roomID = roomId;
}

void Message::SetPacketSize(size_t size)
{
	packetSize = size;
}

uint32 Message::GetMessageDataUInt()
{
	return (uint32)messageData;
}

ushort16 Message::GetMessageDataUShort()
{
	return (ushort16)messageData;
}

std::string Message::GetMessageDataString()
{
	std::string newStr((char*)messageData);
	return newStr;
}

std::string Message::GetRoomId()
{
	return roomID;
}

size_t Message::GetPacketSize()
{
	return packetSize;
}

size_t Message::GetSize()
{
	switch (messageType)
	{
	case Type::String:
		return  ((std::string*)messageData)->length();
		break;
	case Type::Uint:
		return 4;
		break;
	case Type::Ushort:
		return 2;
		break;
	}
}
