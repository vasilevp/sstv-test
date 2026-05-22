#include "robot8.hpp"

#include <cstdint>
#include <vector>

void Robot8::decodeLine(std::span<const float> content)
{
	// The line content between two syncs is exactly the pixel region.
	std::vector<float> luma = sampleChannel(content, 0, content.size());

	std::vector<uint8_t> row(size_t(width) * 3);
	for (uint32_t x = 0; x < width; ++x)
	{
		uint8_t y = uint8_t(luma[x]);
		row[x * 3 + 0] = row[x * 3 + 1] = row[x * 3 + 2] = y; // B/W: R=G=B
	}
	emitRow(row);
}

size_t Robot8::nominalContentSamples() const
{
	return ms2samp(lineTime);
}
