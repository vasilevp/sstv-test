#include "robot8.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <print>
#include <stdexcept>
#include <vector>

#include <LoadBMP/loadbmp.h>

namespace
{
	// A scanline sync pulse is ~5 ms. The accepted window leaves slack for
	// the zero-crossing fuzz at the pulse edges, while still excluding the
	// 10 ms calibration pulse and the 30 ms VIS pulses in the header.
	constexpr float SyncMinMs = 2.0f;
	constexpr float SyncMaxMs = 8.0f;
}

void Robot8::Decode()
{
	if (demod.size() == 0)
		throw std::runtime_error("Empty recording");

	// Identify the mode from the header VIS code. This build only decodes
	// Robot 8 B/W, so the result is informational — a mismatch is reported
	// but decoding still proceeds.
	const VIS vis = detectVIS();
	if (vis.found)
	{
		std::println("VIS code: {} ({}){}", int(vis.code), sstv::visModeName(vis.code),
		             vis.parityOK ? "" : "  [PARITY MISMATCH]");
		if (!vis.parityOK)
			std::println("  warning: VIS parity check failed; the recording may be corrupt");
	}
	else
	{
		std::println("VIS code: not found; assuming Robot 8 B/W");
	}

	// 1. Find every run of samples whose frequency dips into the sync band.
	//    Leading/trailing silence reads as 0 Hz and is skipped by syncRuns().
	const std::vector<Run> runs = syncRuns();

	// 2. Classify the runs. Scanline syncs for lines 1..N-1 show up as short
	//    runs. Line 0's sync is special: the encoder writes the 30 ms VIS
	//    stop marker immediately before it, both at 1200 Hz, so they merge
	//    into one long run. That merged run's end is where line 0's pixels
	//    begin.
	std::vector<Run> shortRuns;
	std::optional<size_t> line0Start;
	for (const Run &r : runs)
	{
		float ms = demod.samp2ms(r.end - r.begin);
		if (ms >= SyncMinMs && ms <= SyncMaxMs)
			shortRuns.push_back(r);
		else if (shortRuns.empty())
			line0Start = r.end; // last long run before the first scanline sync
	}

	// Build one synthetic sync run per scanline. Entry 0 is degenerate: only
	// its `end` (where line 0's pixels start) is ever read.
	std::vector<Run> syncs;
	if (line0Start)
		syncs.push_back({*line0Start, *line0Start});
	syncs.insert(syncs.end(), shortRuns.begin(), shortRuns.end());

	if (syncs.empty())
		throw std::runtime_error("No scanline sync pulses found");

	const uint32_t height = uint32_t(syncs.size());
	std::println("Robot 8 B/W: {} scanlines x {} px", height, width);

	// 3. For each scanline, sample the pixel tones between the end of its
	//    sync pulse and the start of the next one. Using the next sync as
	//    the line boundary makes the sampling self-correct for any timing
	//    drift; the final line falls back to the nominal line duration.
	std::vector<uint8_t> image(size_t(width) * height * 3);

	for (uint32_t line = 0; line < height; ++line)
	{
		size_t pixelBegin = syncs[line].end;
		size_t pixelEnd = (line + 1 < height)
		                      ? syncs[line + 1].begin
		                      : pixelBegin + demod.ms2samp(lineTime);
		if (pixelEnd <= pixelBegin)
			pixelEnd = pixelBegin + demod.ms2samp(lineTime);
		const size_t span = pixelEnd - pixelBegin;

		for (uint32_t x = 0; x < width; ++x)
		{
			// Pixel x occupies an even slice of the scanline's pixel region.
			size_t a = pixelBegin + span * x / width;
			size_t b = pixelBegin + span * (x + 1) / width;
			if (b <= a)
				b = a + 1;

			// Invert the encoder's luma -> frequency mapping.
			float f = demod.average(a, b);
			float luma = (f - sstv::Black) / (sstv::White - sstv::Black) * 255.0f;
			uint8_t y = uint8_t(std::clamp(luma, 0.0f, 255.0f));

			size_t off = (size_t(line) * width + x) * 3;
			image[off] = image[off + 1] = image[off + 2] = y; // B/W: R=G=B
		}
	}

	// 4. Write the reconstructed image.
	if (unsigned err = loadbmp_encode_file(
	        output.c_str(), image.data(), width, height, LOADBMP_RGB))
		throw std::runtime_error("Failed to write BMP (loadbmp error " + std::to_string(err) + ")");

	std::println("Wrote {}", output);
}
