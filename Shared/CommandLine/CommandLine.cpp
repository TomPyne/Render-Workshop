#include "CommandLine.h"

#include "Logging/Logging.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <unordered_map>

namespace
{
	struct CommandLineState_s
	{
		std::unordered_map<std::string, std::string> Args;
		CommandLineArgBase_c* RegisteredHead = nullptr;
		bool bParsed = false;
	};

	// Function-local so declared args can register from any static initialiser
	CommandLineState_s& GetState()
	{
		static CommandLineState_s State;
		return State;
	}

	std::string ToLower(std::string_view Str)
	{
		std::string Lower(Str);
		std::transform(Lower.begin(), Lower.end(), Lower.begin(), [](char C) { return (char)std::tolower((unsigned char)C); });
		return Lower;
	}

	template<typename T>
	bool ParseNumber(std::string_view Str, T& Out)
	{
		T Parsed = {};
		const char* End = Str.data() + Str.size();
		const std::from_chars_result Result = std::from_chars(Str.data(), End, Parsed);
		if (Result.ec != std::errc() || Result.ptr != End)
			return false;

		Out = Parsed;
		return true;
	}
}

void CommandLine_s::Parse(int argc, char* argv[])
{
	CommandLineState_s& State = GetState();
	State.Args.clear();

	bool bHelp = false;

	// argv[0] is the executable
	for (int ArgIndex = 1; ArgIndex < argc; ++ArgIndex)
	{
		std::string_view Arg = argv[ArgIndex];
		if (Arg.empty() || Arg[0] != '-')
		{
			LOGWARNING("Command line: ignoring '%s', args must start with '-'", argv[ArgIndex]);
			continue;
		}

		Arg.remove_prefix(Arg.starts_with("--") ? 2 : 1);

		const size_t Equals = Arg.find('=');
		const std::string Name = ToLower(Arg.substr(0, Equals));
		const std::string Value = Equals == std::string_view::npos ? std::string() : std::string(Arg.substr(Equals + 1));

		if (Name == "help")
		{
			bHelp = true;
			continue;
		}

		State.Args[Name] = Value;
	}

	State.bParsed = true;

	for (CommandLineArgBase_c* RegisteredArg = State.RegisteredHead; RegisteredArg; RegisteredArg = RegisteredArg->Next)
	{
		RegisteredArg->Resolve(Find(RegisteredArg->GetName()));
	}

	for (const auto& [Name, Value] : State.Args)
	{
		bool bKnown = false;
		for (const CommandLineArgBase_c* RegisteredArg = State.RegisteredHead; RegisteredArg && !bKnown; RegisteredArg = RegisteredArg->Next)
		{
			bKnown = ToLower(RegisteredArg->GetName()) == Name;
		}

		CLOGWARNING(!bKnown, "Command line: unknown arg '-%s', use -help to list args", Name.c_str());
	}

	if (bHelp)
	{
		LogHelp();
	}
}

bool CommandLine_s::IsParsed()
{
	return GetState().bParsed;
}

const std::string* CommandLine_s::Find(std::string_view Name)
{
	const CommandLineState_s& State = GetState();
	const auto It = State.Args.find(ToLower(Name));
	return It != State.Args.end() ? &It->second : nullptr;
}

void CommandLine_s::LogHelp()
{
	LOGINFO("Command line args:");
	for (const CommandLineArgBase_c* RegisteredArg = GetState().RegisteredHead; RegisteredArg; RegisteredArg = RegisteredArg->Next)
	{
		if (RegisteredArg->IsSwitch())
		{
			LOGINFO("  -%s : %s", RegisteredArg->GetName(), RegisteredArg->GetHelp());
		}
		else
		{
			LOGINFO("  -%s=<value> : %s (default %s)", RegisteredArg->GetName(), RegisteredArg->GetHelp(), RegisteredArg->GetDefaultString().c_str());
		}
	}
}

CommandLineArgBase_c::CommandLineArgBase_c(const char* InName, const char* InHelp)
	: Name(InName)
	, Help(InHelp)
{
}

void CommandLineArgBase_c::WarnBadValue(const std::string& RawValue) const
{
	LOGWARNING("Command line: invalid value '%s' for '-%s'", RawValue.c_str(), Name);
}

void CommandLineArgBase_c::CheckParsed() const
{
	ASSERTMSG(CommandLine_s::IsParsed(), "Command line arg '-%s' read before CommandLine_s::Parse", Name);
}

void CommandLineArgBase_c::Register()
{
	CommandLineState_s& State = GetState();
	Next = State.RegisteredHead;
	State.RegisteredHead = this;

	if (State.bParsed)
	{
		Resolve(CommandLine_s::Find(Name));
	}
}

bool ParseCommandLineValue(std::string_view Str, int& Out)
{
	return ParseNumber(Str, Out);
}

bool ParseCommandLineValue(std::string_view Str, float& Out)
{
	return ParseNumber(Str, Out);
}

bool ParseCommandLineValue(std::string_view Str, bool& Out)
{
	const std::string Lower = ToLower(Str);
	if (Lower == "1" || Lower == "true")
	{
		Out = true;
		return true;
	}
	if (Lower == "0" || Lower == "false")
	{
		Out = false;
		return true;
	}
	return false;
}

bool ParseCommandLineValue(std::string_view Str, std::string& Out)
{
	Out = Str;
	return true;
}
