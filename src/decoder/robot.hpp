#pragma once
#include <cstdint>
#include <string>

#include "decoder.hpp"

// Shared decoder for the colour Robot modes (Robot 36 and Robot 72).
//
// Mirrors the encoder's Robot base class: every colour-Robot scanline has the
// same layout — a 9 ms sync, a 3 ms porch, a full-width Y channel, then one or
// two chroma channels each introduced by a half-width separator and porch. The
// modes differ only in the Y-channel duration and whether a line carries one
// chroma channel (Robot 36, alternating R-Y / B-Y) or both (Robot 72).
class Robot : public Decoder
{
public:
	void Decode() override;

protected:
	Robot(const std::string &input,
	      const std::string &output,
	      uint32_t width,
	      float lineTime,
	      bool fullColor)
		: Decoder(input, output),
		  width(width),
		  lineTime(lineTime),
		  fullColor(fullColor)
	{
	}

private:
	// Sync pulse / porch durations, standardised across colour Robot modes.
	static constexpr float syncPulse = 9.0f;
	static constexpr float syncPorch = 3.0f;

	uint32_t width;
	float lineTime; // Y-channel duration of one scanline, milliseconds
	bool fullColor; // true: each line carries both chroma channels (Robot 72)
};
