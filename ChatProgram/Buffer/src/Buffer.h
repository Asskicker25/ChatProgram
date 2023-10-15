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

	Buffer(size_t size = 0);
	~Buffer();

	size_t HandlePacketSize(Message& message);

	char* GetBufferData();
	std::vector<uint8> GetBufferDataVector();

	void AddBufferData(std::vector<uint8> addBufferData, int begin, int end);

	void GrowSize(size_t newSize);
	int GetWriteIndex();
	int GetReadIndex();

	size_t GetBufferSize();

	void WriteMessage(Message& message);
	Message ReadMessage();

	void WriteUInt32BE(uint32 value);
	uint32 ReadUInt32BE();

	void WriteUShort16BE(ushort16 value);
	ushort16 ReadUShort16BE();

	void WriteString(std::string value);
	std::string ReadString();

};

