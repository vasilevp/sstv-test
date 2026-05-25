#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "decoder.hpp"

// Streaming decoder for the Scottie colour modes (S1, VIS 60; S2, VIS 56;
// DX, VIS 76).
//
// Scottie sends three full-width raw-RGB channels per line, but — unusually —
// the sync pulse falls between the blue and red channels rather than at the
// line start. So the segment between two syncs holds the red of one line
// followed by the green and blue of the next. The decoder buffers each
// segment's green/blue and pairs them with the red that arrives in the
// following segment. The modes differ only in line duration.
class Scottie : public Decoder
{
public:
	Scottie(const std::string &output, uint32_t width,
	        std::unique_ptr<Demodulator> demod, float lineTime,
	        SimpleMovingAverage syncFilter = SimpleMovingAverage(1))
		: Decoder(output, width, std::move(demod), std::move(syncFilter)),
		  lineTime(lineTime)
	{
	}

protected:
	void decodeLine(std::span<const float> content) override;
	size_t nominalContentSamples() const override;

private:
	static constexpr float syncPorch = 1.5f; // porch before each channel

	float lineTime;  // per-channel pixel duration, milliseconds
	size_t segment = 0; // count of segments seen; segment 0 has no red

	// Green/blue of the line whose red has not arrived yet.
	std::vector<float> pendingG, pendingB;
};
