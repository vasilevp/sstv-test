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
	Robot72(std::unique_ptr<RowSink> sink, uint32_t width,
	        std::unique_ptr<Demodulator> demod,
	        bool cadenceLock = false)
		: Robot(std::move(sink), width, std::move(demod), cadenceLock,
		        138.0f, true)
	{
	}
};
