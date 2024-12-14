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
		// Whether to send a 16-line calibration gradient at the start.
		bool sendCalibrationStrip = false)
		: Robot(input, output, 12, lineTime, sendCalibrationStrip ? greyscaleLines : 0, true) {};

private:
	static constexpr float lineTime = 138;
	static constexpr int greyscaleLines = 16;
};
