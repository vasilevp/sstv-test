#pragma once
#include <string>

#include "robot.hpp"

class Synthesizer;
class Robot72 : public Robot
{
public:
	Robot72(
		// Input file name.
		const std::string &input,
		// Output synthesizer.
		Synthesizer &&s,
		// Greeting text.
		const std::string &greeting = "")
		: Robot(input, std::move(s), 12, lineTime, greeting, true)
	{
		utils::Guard();
	};

private:
	static constexpr float lineTime = 138;
	static constexpr int greetingLines = 16;
};
