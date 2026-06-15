#pragma once
#include <cstdint>
#include <string>

#include "encoder.hpp"

// Encoder for the PD family (Paul Turner, G3WDI). One sync pulse delimits a
// *pair* of scanlines, and a single set of chroma channels is averaged across
// the pair. Each line pair is sent as four equal-time channels in the order
// Y_a, R-Y, B-Y, Y_b. `channelTime` parameterises the mode — 121.6 ms for
// PD120 (VIS 95).
class PD : Encoder
{
public:
	PD(PixelSource &source,
	   Synthesizer &&s,
	   // Per-channel pixel duration, milliseconds.
	   float channelTime = 121.6f,
	   uint8_t visCode = 95,
	   const std::string &greeting = "")
		: Encoder(source, std::move(s), visCode),
		  channelTime(channelTime),
		  greeting(greeting)
	{
		utils::Guard();
	}

	void Encode();

private:
	static constexpr float syncPulse = 20.0f;
	static constexpr float syncPorch = 2.08f;

	const float channelTime;
	const std::string &greeting;

	void writeGreeting();
};
