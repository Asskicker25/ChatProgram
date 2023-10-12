#include "Message.h"
#include <string>

size_t Message::GetSize()
{
	switch (messageType)
	{
		case Type::String:
			return  ((std::string*)messageData)->length();
			break;
		case Type::Uint:
			return 4;
			break;
		case Type::Ushort:
			return 2;
			break;
	}
}
