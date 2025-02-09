#pragma once

#include <chrono>

struct ScopeTimer_s
{
	ScopeTimer_s(const std::wstring& InMessage);
	ScopeTimer_s(const std::string& InMessage);

	~ScopeTimer_s();

private:

	// Assumes that the string is declared in the same scope as the timer and will remain valid
	std::string Message;

	using TimePoint = std::chrono::high_resolution_clock::time_point;
	TimePoint StartTime;
};