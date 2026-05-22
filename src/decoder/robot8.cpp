#include "robot8.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

void Robot8::decodeLine(std::span<const float> content)
{
	// The line content between two syncs is exactly the pixel region:
	// slice it into `width` even pixel slots and invert the encoder's
	// luma -> frequency mapping.
	std::vector<uint8_t> row(size_t(width) * 3);

	for (uint32_t x = 0; x < width; ++x)
	{
		size_t a = content.size() * x / width;
		size_t b = content.size() * (x + 1) / width;
		if (b <= a)
			b = a + 1;

		double acc = 0.0;
		size_t n = 0;
		for (size_t i = a; i < b && i < content.size(); ++i, ++n)
			acc += content[i];
		float f = n ? float(acc / double(n)) : 0.0f;

		float luma = (f - sstv::Black) / (sstv::White - sstv::Black) * 255.0f;
		uint8_t y = uint8_t(std::clamp(luma, 0.0f, 255.0f));
		row[x * 3 + 0] = row[x * 3 + 1] = row[x * 3 + 2] = y; // B/W: R=G=B
	}

	emitRow(row);
}

size_t Robot8::nominalContentSamples() const
{
	return ms2samp(lineTime);
}
