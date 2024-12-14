#pragma once
#include <string>

#include "encoder.hpp"

class Robot8 : Encoder
{
public:
	Robot8(
		// Input file name.
		const std::string &input,
		// Output file name.
		const std::string &output,
		// Whether to send an 8-line calibration gradient at the start.
		bool sendCalibrationStrip = false,
		// Sync pulse time.
		float syncTime = 5,
		// Scanline time.
		float lineTime = 56)
		: Encoder(input, output, 1),
		  greyscaleLines(sendCalibrationStrip ? 8 : 0) {};

	void Encode();

private:
	void writeGreyscale();

	const float syncTime = 5;
	const float lineTime = 56;
	const size_t greyscaleLines;
};
