#include "ScopeTimer.h"

#include "Logging/Logging.h"

using HighResolutionClock = std::chrono::high_resolution_clock;
using Duration = std::chrono::high_resolution_clock::duration;

ScopeTimer_s::ScopeTimer_s(const char* InMessage)
	: Message(InMessage)
{
	StartTime = HighResolutionClock::now();
}

ScopeTimer_s::~ScopeTimer_s()
{
	TimePoint CurrentTime = HighResolutionClock::now();
	Duration DeltaTime = CurrentTime - StartTime;

	float Seconds = static_cast<float>(DeltaTime.count() * 1e-9);

	LOGINFO("[%.2f seconds] %s", Seconds, Message);
}
