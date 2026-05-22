#include "robot.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

void Robot::decodeLine(std::span<const float> content)
{
	// Within-line layout. The chroma channels are introduced by a half-width
	// separator and porch, and each occupies half the Y-channel duration.
	constexpr float chromaSep = syncPulse / 2;
	constexpr float chromaPorch = syncPorch / 2;

	// Reads `width` values from a channel at sample offset `off` spanning
	// `span` samples, inverting the encoder's value -> frequency mapping.
	auto sampleRow = [&](size_t off, size_t span, std::vector<float> &row)
	{
		row.resize(width);
		for (uint32_t x = 0; x < width; ++x)
		{
			size_t a = off + span * x / width;
			size_t b = off + span * (x + 1) / width;
			if (b <= a)
				b = a + 1;

			double acc = 0.0;
			size_t n = 0;
			for (size_t i = a; i < b && i < content.size(); ++i, ++n)
				acc += content[i];
			float f = n ? float(acc / double(n)) : 0.0f;
			row[x] = std::clamp((f - sstv::Black) / (sstv::White - sstv::Black) * 255.0f,
			                    0.0f, 255.0f);
		}
	};

	std::vector<float> Y, Cr, Cb;
	sampleRow(ms2samp(syncPorch), ms2samp(lineTime), Y);

	const size_t cSpan = ms2samp(lineTime / 2);
	const size_t c1 = ms2samp(syncPorch + lineTime + chromaSep + chromaPorch);

	if (fullColor)
	{
		// Robot 72: the line carries R-Y then B-Y back to back.
		const size_t c2 = c1 + cSpan + ms2samp(chromaSep + chromaPorch);
		sampleRow(c1, cSpan, Cr);
		sampleRow(c2, cSpan, Cb);
	}
	else
	{
		// Robot 36: even lines carry R-Y, odd lines B-Y. The missing
		// channel is borrowed from the previous line of opposite parity.
		if (lineIndex % 2 == 0)
		{
			sampleRow(c1, cSpan, Cr);
			Cb = lastCb;
			lastCr = Cr;
		}
		else
		{
			sampleRow(c1, cSpan, Cb);
			Cr = lastCr;
			lastCb = Cb;
		}
	}

	// Convert YCbCr back to RGB. These are the BT.601 studio-swing inverse
	// coefficients matching the encoder's getY / getChroma* matrix. A chroma
	// channel not yet available (the very first Robot 36 line) is neutral.
	std::vector<uint8_t> row(size_t(width) * 3);
	auto px = [](float v) { return uint8_t(std::clamp(v, 0.0f, 255.0f)); };

	for (uint32_t x = 0; x < width; ++x)
	{
		float c = Y[x] - 16.0f;
		float d = (Cb.empty() ? 128.0f : Cb[x]) - 128.0f;
		float e = (Cr.empty() ? 128.0f : Cr[x]) - 128.0f;

		row[x * 3 + 0] = px((298.082f * c + 408.583f * e) / 256.0f);
		row[x * 3 + 1] = px((298.082f * c - 100.291f * d - 208.120f * e) / 256.0f);
		row[x * 3 + 2] = px((298.082f * c + 516.412f * d) / 256.0f);
	}

	emitRow(row);
	++lineIndex;
}

size_t Robot::nominalContentSamples() const
{
	constexpr float chromaSep = syncPulse / 2;
	constexpr float chromaPorch = syncPorch / 2;
	const float chromaCount = fullColor ? 2.0f : 1.0f;
	return ms2samp(syncPorch + lineTime +
	               chromaCount * (chromaSep + chromaPorch + lineTime / 2));
}
