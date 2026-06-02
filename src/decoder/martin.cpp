#include "martin.hpp"

#include <cstdint>
#include <vector>

void Martin::decodeLine(std::span<const float> content)
{
	// One channel's pixel data lasts lineTime * (3 - mode): twice lineTime
	// for M1, once for M2. Each channel is preceded by a porch.
	const float channelTime = lineTime * float(3 - mode);
	const size_t cSpan = ms2samp(channelTime);

	// The content between two syncs is the three raw-RGB channels in the
	// order the encoder sends them: green, blue, red.
	std::vector<float> G = sampleChannel(content, ms2samp(syncPorch), cSpan);
	std::vector<float> B = sampleChannel(content, ms2samp(2 * syncPorch + channelTime), cSpan);
	std::vector<float> R = sampleChannel(content, ms2samp(3 * syncPorch + 2 * channelTime), cSpan);

	std::vector<uint8_t> row(size_t(width) * 3);
	for (uint32_t x = 0; x < width; ++x)
	{
		row[x * 3 + 0] = uint8_t(R[x]);
		row[x * 3 + 1] = uint8_t(G[x]);
		row[x * 3 + 2] = uint8_t(B[x]);
	}
	emitRow(row);
}

size_t Martin::nominalContentSamples() const
{
	// Three porch+channel groups plus the trailing separator porch.
	const float channelTime = lineTime * float(3 - mode);
	return ms2samp(4 * syncPorch + 3 * channelTime);
}

size_t Martin::nominalLinePeriodSamples() const
{
	return ms2samp(syncPulse) + nominalContentSamples();
}
