#pragma once
#include <cstdint>
#include <string>

#include "robot.hpp"

// Robot 36 (VIS 8): colour, ~36 s per frame. Each scanline carries the Y
// channel plus a single chroma channel, alternating R-Y on even lines and
// B-Y on odd lines.
class Robot36 : public Robot
{
public:
	Robot36(std::unique_ptr<RowSink> sink, uint32_t width,
	        std::unique_ptr<Demodulator> demod,
	        bool cadenceLock = false)
		: Robot(std::move(sink), width, std::move(demod), cadenceLock,
		        88.0f, false)
	{
	}
};
