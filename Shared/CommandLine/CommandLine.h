#pragma once

#include <format>
#include <string>
#include <string_view>

// Command line syntax: "-name" for switches, "-name=value" for values. Names are case-insensitive.
// Args are declared as namespace-scope statics so they register before main:
//
//	static CommandLineSwitch_c Windowed("windowed", "Run in a window");
//	static CommandLineValue_c<int> WindowHeight("windowheight", 800, "Window height");

struct CommandLine_s
{
	// Call first thing in main, resolves every declared arg
	static void Parse(int argc, char* argv[]);
	static bool IsParsed();

	// Raw value of an arg as given, nullptr if not given. Switches given without a value return an empty string.
	static const std::string* Find(std::string_view Name);

	static void LogHelp();
};

class CommandLineArgBase_c
{
public:

	CommandLineArgBase_c(const char* InName, const char* InHelp);
	virtual ~CommandLineArgBase_c() = default;

	CommandLineArgBase_c(const CommandLineArgBase_c&) = delete;
	CommandLineArgBase_c& operator=(const CommandLineArgBase_c&) = delete;

	const char* GetName() const { return Name; }
	const char* GetHelp() const { return Help; }

	virtual bool IsSwitch() const = 0;
	virtual std::string GetDefaultString() const = 0;

protected:

	friend struct CommandLine_s;

	virtual void Resolve(const std::string* RawValue) = 0;

	void WarnBadValue(const std::string& RawValue) const;
	void CheckParsed() const;

	// Resolves immediately if constructed after Parse, e.g. a function-local static
	void Register();

private:

	const char* Name;
	const char* Help;
	CommandLineArgBase_c* Next = nullptr;
};

bool ParseCommandLineValue(std::string_view Str, int& Out);
bool ParseCommandLineValue(std::string_view Str, float& Out);
bool ParseCommandLineValue(std::string_view Str, bool& Out);
bool ParseCommandLineValue(std::string_view Str, std::string& Out);

class CommandLineSwitch_c : public CommandLineArgBase_c
{
public:

	CommandLineSwitch_c(const char* InName, const char* InHelp)
		: CommandLineArgBase_c(InName, InHelp)
	{
		Register();
	}

	bool Get() const { CheckParsed(); return bSet; }
	explicit operator bool() const { return Get(); }

	bool IsSwitch() const override { return true; }
	std::string GetDefaultString() const override { return "false"; }

protected:

	// Also accepts an explicit bool, e.g. "-windowed=false"
	void Resolve(const std::string* RawValue) override
	{
		bSet = false;
		if (!RawValue)
			return;

		if (RawValue->empty())
		{
			bSet = true;
		}
		else if (!ParseCommandLineValue(*RawValue, bSet))
		{
			WarnBadValue(*RawValue);
			bSet = true;
		}
	}

private:

	bool bSet = false;
};

template<typename T>
class CommandLineValue_c : public CommandLineArgBase_c
{
public:

	CommandLineValue_c(const char* InName, T InDefault, const char* InHelp)
		: CommandLineArgBase_c(InName, InHelp)
		, Default(InDefault)
		, Value(InDefault)
	{
		Register();
	}

	const T& Get() const { CheckParsed(); return Value; }
	const T& GetDefault() const { return Default; }

	// True if given on the command line with a valid value
	bool IsSet() const { CheckParsed(); return bSet; }

	bool IsSwitch() const override { return false; }
	std::string GetDefaultString() const override { return std::format("{}", Default); }

protected:

	void Resolve(const std::string* RawValue) override
	{
		Value = Default;
		bSet = false;
		if (!RawValue)
			return;

		if (ParseCommandLineValue(*RawValue, Value))
		{
			bSet = true;
		}
		else
		{
			Value = Default;
			WarnBadValue(*RawValue);
		}
	}

private:

	T Default;
	T Value;
	bool bSet = false;
};
