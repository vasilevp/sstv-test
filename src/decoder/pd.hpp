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
	   uint32_t sampleRate,
	   float channelTime = 121.6f)
		: Decoder(output, width, sampleRate),
		  channelTime(channelTime)
	{
	}

protected:
	void decodeLine(std::span<const float> content) override;
	size_t nominalContentSamples() const override;

private:
	static constexpr float syncPorch = 2.08f; // porch after the pair's sync

	float channelTime; // per-channel duration, milliseconds
};
