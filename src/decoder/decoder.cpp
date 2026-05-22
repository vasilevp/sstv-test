#include "decoder.hpp"

#include <algorithm>
#include <bit>
#include <print>
#include <stdexcept>
#include <string>

#include <LoadBMP/loadbmp.h>

// Maps the VIS codes this project's encoder emits to mode names. Codes
// 8/12/40/44/56/60/76 are the standard SSTV assignments; code 1 is the
// non-standard value the encoder uses for its Robot 8 B/W mode.
const char *sstv::visModeName(uint8_t code)
{
	switch (code)
	{
	case 1:  return "Robot 8 B/W";
	case 8:  return "Robot 36";
	case 12: return "Robot 72";
	case 40: return "Martin 2";
	case 44: return "Martin 1";
	case 56: return "Scottie 2";
	case 60: return "Scottie 1";
	case 76: return "Scottie DX";
	default: return "unknown";
	}
}

namespace
{
	// A contiguous run of sync-band frequency samples.
	struct Run
	{
		size_t begin, end;
	};

	// Every sync-band run (frequency below SyncThreshold) in a frequency
	// buffer, in order.
	std::vector<Run> syncRuns(const std::vector<float> &freq)
	{
		std::vector<Run> runs;
		for (size_t i = 0; i < freq.size();)
		{
			if (freq[i] > 0.0f && freq[i] < sstv::SyncThreshold)
			{
				size_t begin = i;
				while (i < freq.size() && freq[i] > 0.0f && freq[i] < sstv::SyncThreshold)
					++i;
				runs.push_back({begin, i});
			}
			else
			{
				++i;
			}
		}
		return runs;
	}
}

Decoder::VIS Decoder::detectVIS(const std::vector<float> &freq, uint32_t sampleRate)
{
	VIS vis;
	auto samp = [&](float ms) { return size_t(sampleRate * ms / 1000.0f); };

	const std::vector<Run> runs = syncRuns(freq);

	// The header opens with a calibration burst: a 1900 Hz tone, a 10 ms
	// sync pulse, another 1900 Hz tone, then the VIS section. So the
	// calibration pulse is the first sync-band run and the VIS start
	// marker is the very next one.
	size_t cal = runs.size();
	for (size_t i = 0; i < runs.size(); ++i)
	{
		float durMs = 1000.0f * float(runs[i].end - runs[i].begin) / float(sampleRate);
		if (durMs >= 4.0f) // skip sub-millisecond zero-crossing glitches
		{
			cal = i;
			break;
		}
	}
	if (cal + 1 >= runs.size())
		return vis; // header not (yet) recognisable; vis.found stays false

	// The VIS section is ten consecutive 30 ms elements, starting at the
	// start marker run:
	//   [start marker] [7 data bits, LSB first] [parity] [stop marker]
	const size_t visStart = runs[cal + 1].begin;
	constexpr float ElementMs = 30.0f;
	if (visStart + samp(10 * ElementMs) > freq.size())
		return vis; // the buffer does not yet cover the whole VIS section

	// Mean frequency of the central third of VIS element `slot`, sampled
	// clear of the transitions at each element boundary.
	auto element = [&](int slot) -> float
	{
		size_t a = visStart + samp(slot * ElementMs + 10.0f);
		size_t b = visStart + samp(slot * ElementMs + 20.0f);
		double acc = 0.0;
		size_t n = 0;
		for (size_t i = a; i < b && i < freq.size(); ++i, ++n)
			acc += freq[i];
		return n ? float(acc / double(n)) : 0.0f;
	};

	// Data bits: 1100 Hz = 1, 1300 Hz = 0, split at SyncPulse (1200 Hz).
	uint8_t code = 0;
	for (int bit = 0; bit < 7; ++bit)
		if (element(1 + bit) < sstv::SyncPulse)
			code |= uint8_t(1u << bit);

	const bool parityBit = element(8) < sstv::SyncPulse;

	vis.found = true;
	vis.code = code;
	// The encoder writes an even-parity bit: it is 1 iff the code has an
	// odd number of set bits.
	vis.parityOK = (std::popcount(code) & 1) == int(parityBit);
	vis.headerEnd = visStart + samp(10 * ElementMs);
	return vis;
}

Decoder::Decoder(const std::string &output, uint32_t width, uint32_t sampleRate)
	: width(width), sampleRate(sampleRate), demod(sampleRate), output(output)
{
}

void Decoder::feed(std::span<const float> audio)
{
	if (finished)
		return;
	for (float sample : audio)
	{
		processFreq(globalIndex, demod.process(sample));
		++globalIndex;
	}
}

void Decoder::processFreq(size_t index, float freq)
{
	if (state == State::Header)
		processHeader(index, freq);
	else
		processImage(index, freq);
}

void Decoder::processHeader(size_t index, float freq)
{
	(void)index;
	headerBuf.push_back(freq);

	if (headerBuf.size() > ms2samp(5000.0f))
		throw std::runtime_error("No VIS header found in the first 5 s of audio");

	// Poll for a complete VIS code once enough of the header could be
	// present; the header runs ~900 ms before the first scanline.
	if (headerBuf.size() < ms2samp(950.0f) || headerBuf.size() % 128 != 0)
		return;

	VIS v = detectVIS(headerBuf, sampleRate);
	if (!v.found || headerBuf.size() <= v.headerEnd)
		return;

	// The header is fully buffered: lock in the mode and switch to image
	// decoding, replaying the samples already received past the header.
	vis = v;
	std::println("VIS code: {} ({}){}", int(vis.code), sstv::visModeName(vis.code),
	             vis.parityOK ? "" : "  [PARITY MISMATCH]");
	if (!vis.parityOK)
		std::println("  warning: VIS parity check failed; the recording may be corrupt");
	state = State::Image;

	std::vector<float> tail(headerBuf.begin() + vis.headerEnd, headerBuf.end());
	headerBuf.clear();
	headerBuf.shrink_to_fit();
	for (size_t i = 0; i < tail.size(); ++i)
		processImage(vis.headerEnd + i, tail[i]);
}

void Decoder::processImage(size_t index, float freq)
{
	const bool sub = freq > 0.0f && freq < sstv::SyncThreshold;

	// Accumulate frequency samples for the line currently in progress.
	if (haveLine)
		lineBuf.push_back(freq);

	// Track sync-band runs. Past the header, every such run is exactly one
	// scanline sync pulse, so a completed run delimits a scanline.
	if (sub && !inRun)
	{
		inRun = true;
		runBegin = index;
	}
	else if (!sub && inRun)
	{
		inRun = false;
		if (!haveLine)
		{
			// First sync after the header: line 0's content starts here.
			haveLine = true;
			lineStart = index;
			lineBuf.clear();
		}
		else
		{
			// This sync bounds the previous line: [lineStart, runBegin).
			size_t contentLen = runBegin > lineStart ? runBegin - lineStart : 0;
			contentLen = std::min(contentLen, lineBuf.size());
			decodeLine(std::span<const float>(lineBuf.data(), contentLen));
			lineStart = index;
			lineBuf.clear();
		}
	}
}

void Decoder::finish()
{
	if (finished)
		return;
	finished = true;

	if (state != State::Image)
		throw std::runtime_error("Stream ended before a VIS header was decoded");

	// Flush the final scanline, which has no trailing sync to bound it.
	if (haveLine && !lineBuf.empty())
	{
		size_t contentLen = std::min(lineBuf.size(), nominalContentSamples());
		decodeLine(std::span<const float>(lineBuf.data(), contentLen));
	}

	if (imageHeight == 0)
		throw std::runtime_error("No scanlines decoded");

	std::println("{}: {} scanlines x {} px",
	             sstv::visModeName(vis.code), imageHeight, width);

	if (unsigned err = loadbmp_encode_file(output.c_str(), image.data(),
	                                       width, imageHeight, LOADBMP_RGB))
		throw std::runtime_error("Failed to write BMP (loadbmp error " + std::to_string(err) + ")");

	std::println("Wrote {}", output);
}

void Decoder::emitRow(const std::vector<uint8_t> &rgb)
{
	image.insert(image.end(), rgb.begin(), rgb.end());
	++imageHeight;
}
