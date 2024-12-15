#pragma once
#include <string>

#include "encoder.hpp"

class Synthesizer;
class Robot8 : Encoder
{
public:
	Robot8(
		// Input file name.
		const std::string &input,
		// Output file name.
		const Synthesizer &output,
		// Whether to send an 8-line calibration gradient at the start.
		const std::string &greeting = "",
		// Sync pulse time.
		float syncTime = 5,
		// Scanline time.
		float lineTime = 56,
		uint8_t visCode = 1)
		: Encoder(input, output, visCode),
		  syncTime(syncTime),
		  lineTime(lineTime),
		  greeting(greeting) {};

	void Encode();

private:
	void writeGreeting();

	const float syncTime;
	const float lineTime;
	const std::string &greeting;
};
