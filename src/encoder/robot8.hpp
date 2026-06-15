#pragma once
#include <string>

#include "encoder.hpp"

class Synthesizer;
class Robot8 : Encoder
{
public:
	Robot8(PixelSource &source,
		Synthesizer &&output,
		// Whether to send an 8-line calibration gradient at the start.
		const std::string &greeting = "",
		// Sync pulse duration in ms. The handbook (Bruchanov ch. 5) gives
		// only the total line time (60/lpm); the 10 ms default matches the
		// calibration pulse and the Robot colour modes' sync convention.
		float syncTime = 10,
		// Pixel-region duration in ms. 10 + 56.667 = 66.667 ms total,
		// matching the handbook's lpm = 900 for Robot B&W 8.
		float lineTime = 56.667f,
		uint8_t visCode = 1)
		: Encoder(source, std::move(output), visCode),
		  syncTime(syncTime),
		  lineTime(lineTime),
		  greeting(greeting)
	{
		utils::Guard();
	};

	void Encode();

private:
	void writeGreeting();

	const float syncTime;
	const float lineTime;
	const std::string &greeting;
};
