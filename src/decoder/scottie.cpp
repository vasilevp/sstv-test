#include "scottie.hpp"

#include <cstdint>
#include <utility>
#include <vector>

void Scottie::decodeLine(std::span<const float> content)
{
	const size_t cSpan = ms2samp(lineTime);

	if (segment == 0)
	{
		// The first segment after the header carries only the green and blue
		// of line 0 — its red has not been sent yet. Buffer them.
		pendingG = sampleChannel(content, ms2samp(syncPorch), cSpan);
		pendingB = sampleChannel(content, ms2samp(2 * syncPorch + lineTime), cSpan);
		++segment;
		return;
	}

	// A normal segment carries [red of the previous line][green][blue].
	std::vector<float> R = sampleChannel(content, ms2samp(syncPorch), cSpan);
	std::vector<float> G = sampleChannel(content, ms2samp(2 * syncPorch + lineTime), cSpan);
	std::vector<float> B = sampleChannel(content, ms2samp(3 * syncPorch + 2 * lineTime), cSpan);

	// Emit the previous line: its red has just arrived, its green and blue
	// were buffered from the previous segment.
	std::vector<uint8_t> row(size_t(width) * 3);
	for (uint32_t x = 0; x < width; ++x)
	{
		row[x * 3 + 0] = uint8_t(R[x]);
		row[x * 3 + 1] = uint8_t(pendingG.empty() ? 0.0f : pendingG[x]);
		row[x * 3 + 2] = uint8_t(pendingB.empty() ? 0.0f : pendingB[x]);
	}
	emitRow(row);

	pendingG = std::move(G);
	pendingB = std::move(B);
	++segment;
}

size_t Scottie::nominalContentSamples() const
{
	// Three porch+channel groups (red, green, blue).
	return ms2samp(3 * syncPorch + 3 * lineTime);
}
