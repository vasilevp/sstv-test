#pragma once
#include <string>

#include "robot.hpp"

class Robot36 : public Robot
{
public:
	Robot36(
		// Input file name.
		const std::string &input,
		// Output file name.
		const std::string &output,
		// Greeting text.
		const std::string &greeting = "")
		: Robot(input, output, 8, lineTime, greeting, false) {};

private:
	static constexpr float lineTime = 88;
};
