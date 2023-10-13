#include "Buffer.h"

Buffer::Buffer(size_t size)
{
	bufferData.resize(size, 0);
}

Buffer::~Buffer()
{
	bufferData.clear();
}

char* Buffer::GetBufferData()
{
	return (char*)&bufferData[0];
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

size_t Buffer::GetBufferSize()
{
	return bufferData.size();
}

void Buffer::WriteMessage(Message& message)
{
	writeIndex = 0;

	WriteUShort16BE((ushort16)message.messageType);
	WriteUShort16BE((ushort16)message.commandType);

	switch (message.messageType)
	{
		case Message::Type::Uint:
			WriteUInt32BE( ((uint32)message.messageData) );
			break;
		case Message::Type::Ushort:
			WriteUShort16BE( ((ushort16)message.messageData) );
			break;
		case Message::Type::String:
			/*char* value = (char*) message.messageData;
			std::string newStr(value);*/
			WriteString(message.GetMessageDataString());
			break;
	}
}

Message Buffer::ReadMessage()
{
	readIndex = 0;

	Message message;

	message.messageType = static_cast<Message::Type>(ReadUShort16BE());
	message.commandType = static_cast<Message::CommandType>(ReadUShort16BE());

	switch (message.messageType)
	{
	case Message::Type::Uint:
		message.messageData = (uint32*) ReadUInt32BE();
		break;
	case Message::Type::Ushort:
		message.messageData = (ushort16*) ReadUShort16BE();
		break;
	case Message::Type::String:
		/*std::string value = ReadString();
		message.messageData = (char*)value.c_str();*/

		std::string value = ReadString();
		message.messageData = new char[value.length() + 1];
		strcpy_s(static_cast<char*>(message.messageData), value.length() + 1, value.c_str());
		break;
	}

	return message;
}

void Buffer::WriteUInt32BE(uint32 value)
{
	if (writeIndex + sizeof(uint32) > bufferData.size())
	{
		GrowSize(sizeof(uint32));
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
	if (writeIndex + sizeof(ushort16) > bufferData.size())
	{
		GrowSize(sizeof(ushort16));
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
	size_t sizeRequired = 2			//message type size
						+ 2			//command type size
						+ sizeof(uint32) 
						+ value.length();

	if (writeIndex + sizeRequired > bufferData.size())
	{
		GrowSize(sizeRequired);
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
