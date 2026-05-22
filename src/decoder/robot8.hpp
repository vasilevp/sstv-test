#pragma once
#include <cstdint>
#include <string>

#include "decoder.hpp"

// Decoder for the Robot 8 B/W mode produced by this project's encoder.
//
// Robot 8 B/W has no chroma: each scanline is a 5 ms sync pulse followed by a
// run of pixel tones, where 1500..2300 Hz maps linearly to luma 0..255.
//
// `width` defaults to 320 to match the encoder's behaviour of using the
// source image's native width (the bundled colortest.bmp is 320x240); the
// classic Robot 8 standard is 160 px wide. The scanline count is recovered
// from the recording, so image height needs no configuration.
class Robot8 : public Decoder
{
public:
	Robot8(const std::string &input,
		   const std::string &output,
		   uint32_t width = 320,
		   float lineTime = 56.0f)
		: Decoder(input, output),
		  width(width),
		  lineTime(lineTime)
	{
	}

	void Decode() override;

private:
	uint32_t width;
	float lineTime; // pixel-data duration of one scanline, milliseconds
};
