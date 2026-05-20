#include "robot.hpp"

#include <algorithm>
#include <cstdint>
#include <print>
#include <stdexcept>
#include <vector>

#include <LoadBMP/loadbmp.h>

void Robot::Decode()
{
	if (demod.size() == 0)
		throw std::runtime_error("Empty recording");

	// Identify the mode from the header VIS code.
	const VIS vis = detectVIS(demod);
	if (!vis.found)
		throw std::runtime_error("Could not locate the VIS header");
	std::println("VIS code: {} ({})", int(vis.code), sstv::visModeName(vis.code));
	if (!vis.parityOK)
		std::println("  warning: VIS parity check failed; the recording may be corrupt");

	// Locate scanline sync pulses. A colour-Robot scanline opens with a
	// 9 ms / 1200 Hz pulse — the only sub-threshold tone in the line, since
	// the chroma separators sit at 1500/2300 Hz. The encoder writes the
	// 30 ms VIS stop marker straight before line 0's sync, merging them
	// into one run; every run that ends past the VIS header is a scanline
	// sync, and the run's end is where that line's sync porch begins.
	std::vector<size_t> porchStart;
	for (const Run &r : syncRuns(demod))
		if (r.end > vis.headerEnd)
			porchStart.push_back(r.end);

	if (porchStart.empty())
		throw std::runtime_error("No scanline sync pulses found");

	const uint32_t height = uint32_t(porchStart.size());
	std::println("{}: {} scanlines x {} px", sstv::visModeName(vis.code), height, width);

	// Within-line layout. The chroma channels are introduced by a half-width
	// separator and porch, and each occupies half the Y-channel duration.
	constexpr float chromaSep = syncPulse / 2;
	constexpr float chromaPorch = syncPorch / 2;
	const float chromaTime = lineTime / 2;
	const size_t ySpan = demod.ms2samp(lineTime);
	const size_t cSpan = demod.ms2samp(chromaTime);

	// Reads `width` values from the channel spanning [start, start+span)
	// samples, inverting the encoder's value -> frequency mapping.
	auto sampleRow = [&](size_t start, size_t span, std::vector<float> &row)
	{
		row.resize(width);
		for (uint32_t x = 0; x < width; ++x)
		{
			size_t a = start + span * x / width;
			size_t b = start + span * (x + 1) / width;
			if (b <= a)
				b = a + 1;
			float f = demod.average(a, b);
			row[x] = std::clamp((f - sstv::Black) / (sstv::White - sstv::Black) * 255.0f,
			                    0.0f, 255.0f);
		}
	};

	// Decode every scanline into its Y and chroma rows (values 0..255).
	struct Channels
	{
		std::vector<float> Y, Cr, Cb;
	};
	std::vector<Channels> line(height);

	for (uint32_t L = 0; L < height; ++L)
	{
		size_t yStart = porchStart[L] + demod.ms2samp(syncPorch);
		sampleRow(yStart, ySpan, line[L].Y);

		size_t c1 = porchStart[L] +
		            demod.ms2samp(syncPorch + lineTime + chromaSep + chromaPorch);
		if (fullColor)
		{
			// Robot 72: the line carries R-Y then B-Y back to back.
			size_t c2 = c1 + cSpan + demod.ms2samp(chromaSep + chromaPorch);
			sampleRow(c1, cSpan, line[L].Cr);
			sampleRow(c2, cSpan, line[L].Cb);
		}
		else
		{
			// Robot 36: lines alternate; even lines carry R-Y, odd B-Y.
			sampleRow(c1, cSpan, (L % 2 == 0) ? line[L].Cr : line[L].Cb);
		}
	}

	// Robot 36 sends only one chroma channel per line, so each line borrows
	// its missing channel from the neighbouring line of opposite parity —
	// the same pair the encoder averaged the chroma across.
	if (!fullColor)
	{
		for (uint32_t L = 0; L < height; ++L)
		{
			if (line[L].Cr.empty())
			{
				if (L > 0 && !line[L - 1].Cr.empty())
					line[L].Cr = line[L - 1].Cr;
				else if (L + 1 < height && !line[L + 1].Cr.empty())
					line[L].Cr = line[L + 1].Cr;
			}
			if (line[L].Cb.empty())
			{
				if (L + 1 < height && !line[L + 1].Cb.empty())
					line[L].Cb = line[L + 1].Cb;
				else if (L > 0 && !line[L - 1].Cb.empty())
					line[L].Cb = line[L - 1].Cb;
			}
		}
	}

	// Convert YCbCr back to RGB. These are the BT.601 studio-swing inverse
	// coefficients matching the encoder's getY / getChroma* matrix.
	std::vector<uint8_t> image(size_t(width) * height * 3);
	auto px = [](float v) { return uint8_t(std::clamp(v, 0.0f, 255.0f)); };

	for (uint32_t L = 0; L < height; ++L)
	{
		const Channels &ch = line[L];
		for (uint32_t x = 0; x < width; ++x)
		{
			float c = ch.Y[x] - 16.0f;
			float d = (ch.Cb.empty() ? 128.0f : ch.Cb[x]) - 128.0f;
			float e = (ch.Cr.empty() ? 128.0f : ch.Cr[x]) - 128.0f;

			size_t off = (size_t(L) * width + x) * 3;
			image[off + 0] = px((298.082f * c + 408.583f * e) / 256.0f);
			image[off + 1] = px((298.082f * c - 100.291f * d - 208.120f * e) / 256.0f);
			image[off + 2] = px((298.082f * c + 516.412f * d) / 256.0f);
		}
	}

	if (unsigned err = loadbmp_encode_file(
	        output.c_str(), image.data(), width, height, LOADBMP_RGB))
		throw std::runtime_error("Failed to write BMP (loadbmp error " + std::to_string(err) + ")");

	std::println("Wrote {}", output);
}
