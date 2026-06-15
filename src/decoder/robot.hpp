#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "decoder.hpp"

// Shared streaming decoder for the colour Robot modes (Robot 36 and Robot 72).
//
// Mirrors the encoder's Robot base class: every colour-Robot scanline has the
// same layout — a sync, a porch, a full-width Y channel, then one or two
// chroma channels each introduced by a half-width separator and porch. The
// modes differ only in the Y-channel duration and whether a line carries one
// chroma channel (Robot 36, alternating R-Y / B-Y) or both (Robot 72).
class Robot : public Decoder
{
protected:
	Robot(std::unique_ptr<RowSink> sink,
	      uint32_t width,
	      std::unique_ptr<Demodulator> demod,
	      bool cadenceLock,
	      float lineTime,
	      bool fullColor)
		: Decoder(std::move(sink), width, std::move(demod), cadenceLock),
		  lineTime(lineTime),
		  fullColor(fullColor)
	{
	}

	void decodeLine(std::span<const float> content) override;
	size_t nominalContentSamples() const override;
	size_t nominalLinePeriodSamples() const override;

private:
	// Sync pulse / porch durations, standardised across colour Robot modes.
	static constexpr float syncPulse = 9.0f;
	static constexpr float syncPorch = 3.0f;

	float lineTime; // Y-channel duration of one scanline, milliseconds
	bool fullColor; // true: each line carries both chroma channels (Robot 72)

	// Robot 36 sends one chroma channel per line; the streaming decoder
	// reuses the previous line's other channel rather than waiting for the
	// next one, keeping decoding causal.
	size_t lineIndex = 0;
	std::vector<float> lastCr, lastCb;
};
