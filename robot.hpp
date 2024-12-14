#pragma once
#include <string>

#include "encoder.hpp"

class Robot : Encoder
{
public:
	void Encode() override;

protected:
	Robot(
		// Input file name.
		const std::string &input,
		// Output file name.
		const std::string &output,
		// VIS code.
		const int vCode,
		// Scanline length in milliseconds.
		const float lineTime,
		// Number of gradient lines to send before the image.
		const size_t greyscaleLines,
		// Send both color bursts at once instead of alternating between scanlines. Used with the slower modes like Robot 72.
		const bool repeat)
		: Encoder(input, output, vCode),
		  lineTime(lineTime),
		  greyscaleLines(greyscaleLines),
		  repeat(repeat) {};

private:
	void writeGreyscale();

	// Sync pulse length. Standardized at 9ms for all color Robot modes.
	static constexpr float syncPulse = 9;
	// Sync porch length. Standardized at 3ms for all color Robot modes.
	static constexpr float syncPorch = 3;
	const bool repeat;
	const float lineTime;
	const size_t greyscaleLines;
};
