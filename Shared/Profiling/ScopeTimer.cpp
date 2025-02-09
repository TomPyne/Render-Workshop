#include "ScopeTimer.h"

#include "Logging/Logging.h"
#include "StringUtils/StringUtils.h"

using HighResolutionClock = std::chrono::high_resolution_clock;
using Duration = std::chrono::high_resolution_clock::duration;

ScopeTimer_s::ScopeTimer_s(const std::wstring& InMessage)
{
	Message = WideToNarrow(InMessage);

	StartTime = HighResolutionClock::now();
}

ScopeTimer_s::ScopeTimer_s(const std::string& InMessage)
	: Message(InMessage)
{
	StartTime = HighResolutionClock::now();
}

ScopeTimer_s::~ScopeTimer_s()
{
	TimePoint CurrentTime = HighResolutionClock::now();
	Duration DeltaTime = CurrentTime - StartTime;

	float Seconds = static_cast<float>(DeltaTime.count() * 1e-9);

	LOGINFO("[%.2f seconds] %s", Seconds, Message.c_str());
}
