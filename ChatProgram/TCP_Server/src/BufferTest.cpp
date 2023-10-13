#include <iostream>
#include <string>
#include <Buffer.h>

int main()
{
	Buffer b1;
	Buffer b2;
	Buffer b3;

	Message message;

	message.messageType = Message::Type::Uint;
	message.messageData = (uint32*)50;

	b1.WriteMessage(message);

	Message newMessage = b1.ReadMessage();

	std::cout << (uint32)message.messageData << std::endl;

	std::string str;
	str = "Hello World";

	Message message1;
	message1.messageType = Message::Type::String;

	message1.messageData = (void*)str.c_str();

	b1.WriteMessage(message1);

	Message newMessage1 = b1.ReadMessage();

	std::string newString((char*)newMessage1.messageData);

	std::cout << newString << std::endl;

	/*b1.WriteUInt32BE(500);
	printf("Write Index: %d\n", b1.GetWriteIndex());

	b2.WriteUShort16BE(50);
	printf("Write Index: %d\n", b2.GetWriteIndex());

	b3.WriteString("Hello World!");
	printf("Write Index: %d\n", b3.GetWriteIndex());


	uint32 test500 = b1.ReadUInt32BE();
	ushort16 test50 = b2.ReadUShort16BE();
	std::string hello = b3.ReadString();

	printf("test500 : %d\ntest50 : %u\n", test500, test50);
	printf("Hello: %s\n", hello.c_str());*/
}