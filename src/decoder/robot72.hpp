#pragma once
#include <cstdint>
#include <string>

#include "robot.hpp"

// Robot 72 (VIS 12): colour, ~72 s per frame. Each scanline carries the Y
// channel followed by both chroma channels (R-Y then B-Y), so every line is
// independently full colour.
class Robot72 : public Robot
{
public:
	Robot72(const std::string &output, uint32_t width, uint32_t sampleRate)
		: Robot(output, width, sampleRate, /*lineTime=*/138.0f, /*fullColor=*/true)
	{
	}
};
