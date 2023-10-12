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

	Type messageType;
	void* messageData;

	size_t GetSize();
};

