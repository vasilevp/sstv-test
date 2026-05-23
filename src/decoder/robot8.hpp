#pragma once
#include <cstdint>
#include <span>
#include <string>

#include "decoder.hpp"

// Streaming decoder for the Robot 8 B/W mode produced by this project's
// encoder. Each scanline is a sync pulse followed directly by pixel tones,
// where 1500..2300 Hz maps linearly to luma 0..255 (no chroma).
//
// `width` defaults to 320 to match the encoder's behaviour of using the
// source image's native width; the classic Robot 8 standard is 160 px.
class Robot8 : public Decoder
{
public:
	Robot8(const std::string &output,
	       uint32_t width,
	       std::unique_ptr<Demodulator> demod,
	       float lineTime = 56.667f)
		: Decoder(output, width, std::move(demod)),
		  lineTime(lineTime)
	{
	}

protected:
	void decodeLine(std::span<const float> content) override;
	size_t nominalContentSamples() const override;

private:
	float lineTime; // pixel-data duration of one scanline, milliseconds
};
