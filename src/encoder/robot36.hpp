#pragma once
#include <string>

#include "robot.hpp"

class Synthesizer;
class Robot36 : public Robot
{
public:
	Robot36(
		// Input file name.
		const std::string &input,
		// Output file name.
		Synthesizer &&output,
		// Greeting text.
		const std::string &greeting = "")
		: Robot(input, std::move(output), 8, lineTime, greeting, false)
	{
		utils::Guard();
	};

private:
	static constexpr float lineTime = 88;
};
