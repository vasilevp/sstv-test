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
		// Whether to send a 16-line calibration gradient at the start.
		bool sendCalibrationStrip = false)
		: Robot(input, output, 8, lineTime, sendCalibrationStrip ? greyscaleLines : 0, false) {};

private:
	static constexpr float lineTime = 88;
	static constexpr int greyscaleLines = 16;
};
