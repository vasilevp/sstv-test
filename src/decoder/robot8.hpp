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
	Robot8(std::unique_ptr<RowSink> sink,
	       uint32_t width,
	       std::unique_ptr<Demodulator> demod,
	       bool cadenceLock = false,
	       float lineTime = 56.667f)
		: Decoder(std::move(sink), width, std::move(demod), cadenceLock),
		  lineTime(lineTime)
	{
	}

protected:
	void decodeLine(std::span<const float> content) override;
	size_t nominalContentSamples() const override;
	size_t nominalLinePeriodSamples() const override;

private:
	// Sync pulse duration; mirrors the encoder default (Robot8::Robot8 in
	// src/encoder/robot8.hpp). The encoder also takes the figure as a
	// constructor parameter, but the decoder isn't given a way to override
	// it — every recording the encoder produces uses 10 ms.
	static constexpr float syncPulse = 10.0f;

	float lineTime; // pixel-data duration of one scanline, milliseconds
};
