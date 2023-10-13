#include "Events.h"

void Events::Invoke()
{
	std::map<std::string, EventHandler>::iterator it;
	for (it = events.begin(); it != events.end(); ++it)
	{
		it->second();
	}
}

void Events::Addfunction(const std::string& eventName, EventHandler event)
{
	events[eventName] = event;
}

void Events::RemoveFunction(const std::string& eventName)
{
	events.erase(eventName);
}
