#pragma once

class Message
{
public :
	enum Type
	{
		string,
		Uint,
		Ushort
	};

	Type messageType;
	void* messageData;

	size_t GetSize();
};

