#pragma once
#include <cstdint>
#include <string>

#include "robot.hpp"

// Robot 36 (VIS 8): colour, ~36 s per frame. Each scanline carries the Y
// channel plus a single chroma channel, alternating R-Y on even lines and
// B-Y on odd lines; the decoder pairs adjacent lines to recover full colour.
class Robot36 : public Robot
{
public:
	Robot36(const std::string &input, const std::string &output, uint32_t width = 320)
		: Robot(input, output, width, /*lineTime=*/88.0f, /*fullColor=*/false)
	{
	}
};
