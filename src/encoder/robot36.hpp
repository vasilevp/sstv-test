#pragma once
#include <string>

#include "robot.hpp"

class Synthesizer;
class Robot36 : public Robot
{
public:
	Robot36(PixelSource &source,
		Synthesizer &&output,
		const std::string &greeting = "")
		: Robot(source, std::move(output), 8, lineTime, greeting, false)
	{
		utils::Guard();
	};

private:
	static constexpr float lineTime = 88;
};
