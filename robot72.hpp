#pragma once
#include <string>

#include "robot.hpp"

class Robot72 : public Robot
{
public:
	Robot72(
		// Input file name.
		const std::string &input,
		// Output file name.
		const std::string &output,
		// Greeting text.
		const std::string &greeting = "")
		: Robot(input, output, 12, lineTime, greeting, true) {};

private:
	static constexpr float lineTime = 138;
	static constexpr int greetingLines = 16;
};
