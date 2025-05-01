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
		Synthesizer &&s,
		// VIS code.
		const int vCode,
		// Scanline length in milliseconds.
		const float lineTime,
		// Greeting text.
		const std::string &greeting,
		// Send both color bursts at once instead of alternating between scanlines. Used with the slower modes like Robot 72.
		const bool fullColor)
		: Encoder(input, std::move(s), vCode),
		  lineTime(lineTime),
		  greeting(greeting),
		  fullColor(fullColor)
	{
		utils::Guard();
	};

private:
	void writeGreeting();

	// Sync pulse length. Standardized at 9ms for all color Robot modes.
	static constexpr float syncPulse = 9;
	// Sync porch length. Standardized at 3ms for all color Robot modes.
	static constexpr float syncPorch = 3;
	const bool fullColor;
	const float lineTime;
	const std::string &greeting;
};
