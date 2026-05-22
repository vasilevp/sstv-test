#include "pd.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

void PD::decodeLine(std::span<const float> content)
{
	// Between two syncs come four equal-time channels in order: Y_a, R-Y,
	// B-Y, Y_b. The chroma channels are shared by both lines of the pair.
	const size_t cSpan = ms2samp(channelTime);
	const size_t porch = ms2samp(syncPorch);

	std::vector<float> Y_a = sampleChannel(content, porch + 0 * cSpan, cSpan);
	std::vector<float> Cr  = sampleChannel(content, porch + 1 * cSpan, cSpan);
	std::vector<float> Cb  = sampleChannel(content, porch + 2 * cSpan, cSpan);
	std::vector<float> Y_b = sampleChannel(content, porch + 3 * cSpan, cSpan);

	// Convert YCbCr to RGB (BT.601 studio-swing inverse, matching the
	// encoder's getY / getChroma* matrix) and emit both rows of the pair.
	auto px = [](float v) { return uint8_t(std::clamp(v, 0.0f, 255.0f)); };
	auto emit = [&](const std::vector<float> &Y)
	{
		std::vector<uint8_t> row(size_t(width) * 3);
		for (uint32_t x = 0; x < width; ++x)
		{
			const float c = Y[x] - 16.0f;
			const float d = Cb[x] - 128.0f;
			const float e = Cr[x] - 128.0f;
			row[x * 3 + 0] = px((298.082f * c + 408.583f * e) / 256.0f);
			row[x * 3 + 1] = px((298.082f * c - 100.291f * d - 208.120f * e) / 256.0f);
			row[x * 3 + 2] = px((298.082f * c + 516.412f * d) / 256.0f);
		}
		emitRow(row);
	};

	emit(Y_a);
	emit(Y_b);
}

size_t PD::nominalContentSamples() const
{
	// Porch plus four equal-time channels.
	return ms2samp(syncPorch + 4 * channelTime);
}
