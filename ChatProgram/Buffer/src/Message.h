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
		SetName
	};
	
	CommandType commandType;
	Type messageType;
	void* messageData;

	void SetMessageDataUInt(const uint32& value);
	void SetMessageDataUShort(const ushort16& value);
	void SetMessageDataString(const std::string& value);

	uint32 GetMessageDataUInt();
	ushort16 GetMessageDataUShort();
	std::string GetMessageDataString();

	size_t GetSize();
};

