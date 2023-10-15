#include "Buffer.h"

Buffer::Buffer(size_t size)
{
	bufferData.resize(size, -1);
}

Buffer::~Buffer()
{
	bufferData.clear();
}

char* Buffer::GetBufferData()
{
	return (char*)&bufferData[0];
}

std::vector<uint8> Buffer::GetBufferDataVector()
{
	return bufferData;
}

void Buffer::GrowSize(size_t newSize)
{
	bufferData.resize(newSize, 0);
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

void Buffer::AddBufferData(std::vector<uint8> addBufferData, int begin, int end)
{
	bufferData.insert(bufferData.end(), addBufferData.begin() + begin, addBufferData.begin() + end + 1);
}

size_t Buffer::HandlePacketSize(Message& message)
{
	size_t sizeRequired = sizeof(uint32)									//packet size
						+ sizeof(uint32)									//Room length
						+ message.roomID.length()							//Room string length
						+ sizeof(ushort16) 									//message type size
						+ sizeof(ushort16);									//command type size


	switch (message.messageType)
	{
	case Message::Type::Uint:

		sizeRequired += sizeof(uint32);										//Message size
		break;
	case Message::Type::Ushort:

		sizeRequired += sizeof(ushort16);									//Message size
		break;
	case Message::Type::String:

		sizeRequired += sizeof(uint32)										//Message length
						+ message.GetMessageDataString().length();			//message string length
		break;
	}

	if (sizeRequired > bufferData.size())
	{
		GrowSize(sizeRequired);
	}

	return sizeRequired;
}

void Buffer::WriteMessage(Message& message)
{
	writeIndex = 0;

	message.SetPacketSize(HandlePacketSize(message));						

	WriteUInt32BE(message.packetSize);									//Add packet size 

	WriteUShort16BE((ushort16)message.messageType);						//Add message Type
	WriteUShort16BE((ushort16)message.commandType);						//Add command Type

	WriteString(message.GetRoomId());									//Add room length and room string

	switch (message.messageType)
	{
		case Message::Type::Uint:

			WriteUInt32BE( ((uint32)message.messageData) );				//Add message 
			break;
		case Message::Type::Ushort:

			WriteUShort16BE( ((ushort16)message.messageData) );			//Add messagae
			break;
		case Message::Type::String:
			WriteString(message.GetMessageDataString());				//Add message length and message string
			break;
	}
}


Message Buffer::ReadMessage()
{
	readIndex = 0;

	Message message;

	//essage.SetPacketSize(ReadUInt32BE());											//Read the packet size

	message.messageType = static_cast<Message::Type>(ReadUShort16BE());				//Read the message type
	message.commandType = static_cast<Message::CommandType>(ReadUShort16BE());		//Read the command type

	message.roomID = ReadString();													//Read the room string

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

		std::string value = ReadString();											//Read the message string
		message.messageData = new char[value.length() + 1];
		strcpy_s(static_cast<char*>(message.messageData), value.length() + 1, value.c_str());
		break;
	}

	return message;
}

void Buffer::WriteUInt32BE(uint32 value)
{
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
	
	readIndex += length;

	return value;
}
