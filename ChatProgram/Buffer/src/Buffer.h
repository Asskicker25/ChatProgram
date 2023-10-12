#pragma once

#include <vector>
#include <string>

#include "Message.h"

typedef unsigned int uint32;
typedef unsigned short ushort16;
typedef unsigned char uint8;


class Buffer
{
private:

	std::vector<uint8> bufferData;
	int readIndex = 0;
	int writeIndex = 0;

public:
	Buffer(size_t size = 10);
	~Buffer();

	void GrowSize(size_t newSize);
	int GetWriteIndex();
	int GetReadIndex();

	void WriteMessage(Message& message);
	Message ReadMessage();

	void WriteUInt32BE(uint32 value);
	uint32 ReadUInt32BE();

	void WriteUShort16BE(ushort16 value);
	ushort16 ReadUShort16BE();

	void WriteString(std::string value);
	std::string ReadString();

};

