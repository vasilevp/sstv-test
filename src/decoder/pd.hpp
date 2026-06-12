#pragma once
#include <cstdint>
#include <span>
#include <string>

#include "decoder.hpp"

// Streaming decoder for the PD family (Paul Turner, G3WDI). PD sends scanlines
// in pairs: one sync pulse delimits each pair, and the four equal-time
// channels between consecutive syncs are Y_a, R-Y, B-Y, Y_b — with the two
// chroma channels averaged across the pair. The decoder emits *two* RGB rows
// per decodeLine() call, sharing chroma between them.
//
// `channelTime` parameterises the mode: 121.6 ms is PD120 (VIS 95). Other PD
// variants (PD50, PD90, PD180, ...) plug in via different channelTime values.
class PD : public Decoder
{
public:
	PD(const std::string &output,
	   uint32_t width,
	   std::unique_ptr<Demodulator> demod,
	   float channelTime = 121.6f,
	   bool cadenceLock = false)
		: Decoder(output, width, std::move(demod), cadenceLock),
		  channelTime(channelTime)
	{
	}

protected:
	void decodeLine(std::span<const float> content) override;
	size_t nominalContentSamples() const override;
	size_t nominalLinePeriodSamples() const override;

private:
	// Sync pulse duration; mirrors PD::syncPulse in src/encoder/pd.hpp.
	static constexpr float syncPulse = 20.0f;
	static constexpr float syncPorch = 2.08f; // porch after the pair's sync

	float channelTime; // per-channel duration, milliseconds
};
