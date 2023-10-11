#include "Buffer.h"

Buffer::Buffer(size_t size)
{
	bufferData.resize(size, 0);
}

Buffer::~Buffer()
{
	bufferData.clear();
}

void Buffer::GrowSize(size_t icreaseBy)
{
	bufferData.resize(bufferData.size() + icreaseBy, 0);
}

int Buffer::GetWriteIndex()
{
	return writeIndex;
}

int Buffer::GetReadIndex()
{
	return readIndex;
}

void Buffer::WriteUInt32BE(uint32 value)
{
	if (writeIndex + 4 > bufferData.size())
	{
		GrowSize(4);
	}

	bufferData[writeIndex]		= value >> 24;
	bufferData[writeIndex + 1]	= value >> 16;
	bufferData[writeIndex + 2]	= value >> 8;
	bufferData[writeIndex + 3]	= value;

	writeIndex += 4;
}

uint32 Buffer::ReadUInt32BE()
{
	uint32 value;

	value  = bufferData[readIndex] << 24;
	value |= bufferData[readIndex + 1] << 16;
	value |= bufferData[readIndex + 2] << 8;
	value |= bufferData[readIndex + 3];

	readIndex += 4;
	
	return value;
}

void Buffer::WriteUShort16BE(ushort16 value)
{
	if (writeIndex + 2 > bufferData.size())
	{
		GrowSize(2);
	}

	bufferData[writeIndex]		= value >> 8;
	bufferData[writeIndex + 1] = value;

	writeIndex += 2;
}

ushort16 Buffer::ReadUShort16BE()
{
	ushort16 value;

	value = bufferData[readIndex] << 8;
	value |= bufferData[readIndex + 1];

	readIndex += 2;

	return value;
}

void Buffer::WriteString(std::string value)
{
	if (writeIndex + value.length() > bufferData.size())
	{
		GrowSize(4 + value.length());
	}

	WriteUInt32BE(static_cast<uint32>(value.length()));

	const char* data = value.c_str();

	for (int i = 0; i < value.length(); i++)
	{
		bufferData[writeIndex + i] = data[i];
	}

	writeIndex += value.length();
}

std::string Buffer::ReadString()
{

	uint32 length = ReadUInt32BE();

	if (readIndex + length > bufferData.size()) {
		return "";
	}

	std::string value(bufferData.begin() + readIndex, bufferData.begin() + readIndex + length);

	return value;
}
