#pragma once
#include <string>

typedef unsigned int uint32;
typedef unsigned short ushort16;
typedef unsigned char uint8;

class Message
{
public :

	Message()
	{
		commandType = Message::CommandType::Chat;
		messageType = Message::Type::String;
		messageData = nullptr;
	}
	~Message()
	{
	}

	enum Type
	{
		String,
		Uint,
		Ushort
	};

	enum CommandType
	{
		Chat,
		JoinedRoom
	};
	
	size_t packetSize;
	CommandType commandType;
	Type messageType;
	std::string roomID;
	void* messageData;

	void SetMessageDataUInt(const uint32& value);
	void SetMessageDataUShort(const ushort16& value);
	void SetMessageDataString(const std::string& value);
	void SetRoomId(const std::string& roomId);
	void SetPacketSize(size_t size);

	uint32 GetMessageDataUInt();
	ushort16 GetMessageDataUShort();
	std::string GetMessageDataString();
	std::string GetRoomId();
	size_t GetPacketSize();


	size_t GetSize();
};

