#pragma once
#include <cstdint>
#include <span>
#include <string>

#include "decoder.hpp"

// Streaming decoder for the Martin colour modes (M1, VIS 44; M2, VIS 40).
//
// Each scanline is a sync pulse followed by three full-width raw-RGB channels
// — green, blue, red — every channel introduced by a short porch and the line
// closed by a separator porch. M1 scans each channel in twice the time of M2;
// `mode` (1 or 2) selects that.
class Martin : public Decoder
{
public:
	Martin(std::unique_ptr<RowSink> sink, uint32_t width,
	       std::unique_ptr<Demodulator> demod, uint8_t mode,
	       bool cadenceLock = false)
		: Decoder(std::move(sink), width, std::move(demod), cadenceLock),
		  mode(mode)
	{
	}

protected:
	void decodeLine(std::span<const float> content) override;
	size_t nominalContentSamples() const override;
	size_t nominalLinePeriodSamples() const override;

private:
	// Durations standardised across Martin modes, in milliseconds.
	static constexpr float syncPulse = 4.862f;
	static constexpr float syncPorch = 0.572f;
	static constexpr float lineTime = 73.216f;

	uint8_t mode; // 1 or 2; one channel scans for lineTime * (3 - mode)
};
