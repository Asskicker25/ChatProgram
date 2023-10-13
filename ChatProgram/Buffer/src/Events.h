#pragma once

#include <functional>
#include <map>
#include <string>

class Events
{
public:
	using EventHandler = std::function<void()>;
	void Invoke();
	void Addfunction(const std::string& eventName, EventHandler event);
	void RemoveFunction(const std::string& eventName);

private:
	std::map<std::string, EventHandler> events;
};

