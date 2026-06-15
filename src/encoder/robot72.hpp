#pragma once
#include <string>

#include "robot.hpp"

class Synthesizer;
class Robot72 : public Robot
{
public:
	Robot72(RowSource &source,
		Synthesizer &&s,
		const std::string &greeting = "")
		: Robot(source, std::move(s), 12, lineTime, greeting, true)
	{
		utils::Guard();
	};

private:
	static constexpr float lineTime = 138;
	static constexpr int greetingLines = 16;
};
