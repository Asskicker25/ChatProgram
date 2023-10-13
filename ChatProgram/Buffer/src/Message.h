#pragma once

class Message
{
public :
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

	size_t GetSize();
};

