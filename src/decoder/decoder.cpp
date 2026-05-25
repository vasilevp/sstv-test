#include "decoder.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <print>
#include <stdexcept>
#include <string>

#include "simple_moving_average.hpp"

#include <LoadBMP/loadbmp.h>

// SSTV mode names by VIS code, per Bruchanov, "Image Communication on Short
// Waves" (sstv-handbook.com), chapter 5. The Robot B&W and Robot Color VIS
// codes come in triplets/groups for separate R/G/B filter components — they
// all denote the same mode, just a different colour channel selection.
const char *sstv::visModeName(uint8_t code)
{
	switch (code)
	{
	case 0:
		return "Robot Color 12";
	case 1:
	case 2:
	case 3:
		return "Robot B&W 8";
	case 4:
		return "Robot Color 24";
	case 5:
	case 6:
	case 7:
		return "Robot B&W 12";
	case 8:
		return "Robot Color 36";
	case 9:
	case 10:
	case 11:
		return "Robot B&W 24";
	case 12:
		return "Robot Color 72";
	case 13:
	case 14:
	case 15:
		return "Robot B&W 36";
	case 32:
		return "Martin M4";
	case 36:
		return "Martin M3";
	case 40:
		return "Martin M2";
	case 44:
		return "Martin M1";
	case 48:
		return "Scottie S4";
	case 52:
		return "Scottie S3";
	case 56:
		return "Scottie S2";
	case 60:
		return "Scottie S1";
	case 76:
		return "Scottie DX";
	case 93:
		return "PD 50";
	case 94:
		return "PD 290";
	case 95:
		return "PD 120";
	case 96:
		return "PD 180";
	case 97:
		return "PD 240";
	case 98:
		return "PD 160";
	case 99:
		return "PD 90";
	default:
		return "unknown";
	}
}

namespace
{
	// A contiguous run of sync-band frequency samples.
	struct Run
	{
		size_t begin, end;
	};

	// Every sync-band run (frequency below the Schmitt trigger's enter
	// threshold) in a frequency buffer. `syncFilterWindow` selects the
	// pre-trigger SMA size; 1 = identity (raw freq goes straight to the
	// trigger, preserving edge timing on clean signals).
	std::vector<Run> syncRuns(const std::vector<float> &freq,
	                          std::size_t syncFilterWindow)
	{
		std::vector<Run> runs;
		SimpleMovingAverage sma(std::max<std::size_t>(1, syncFilterWindow));
		SchmittTrigger trig(sstv::SyncEnterHz, sstv::SyncExitHz);
		bool inRun = false;
		size_t runBegin = 0;
		for (size_t i = 0; i < freq.size(); ++i)
		{
			const bool sub = (freq[i] > 0.0f) && trig.update(sma.update(freq[i]));
			if (sub && !inRun)
			{
				inRun = true;
				runBegin = i;
			}
			else if (!sub && inRun)
			{
				inRun = false;
				runs.push_back({runBegin, i});
			}
		}
		if (inRun)
			runs.push_back({runBegin, freq.size()});
		return runs;
	}
}

std::size_t Decoder::recommendedSyncFilterWindow(std::uint32_t sampleRate)
{
	// ~1.25 ms — long enough to integrate single-sample in-band noise
	// spikes away, short enough that the smoothing-induced content-
	// dependent edge shift stays small. Round to an odd window so the
	// group delay is an integer number of samples (matches xdsopl's
	// `| 1` idiom).
	constexpr float SyncFilterMs = 1.25f;
	std::size_t n = std::max<std::size_t>(1, std::size_t(SyncFilterMs * sampleRate / 1000.0f));
	return n | 1u;
}

Decoder::VIS Decoder::detectVIS(const std::vector<float> &freq, uint32_t sampleRate,
                                std::size_t syncFilterWindow)
{
	VIS vis;
	auto samp = [&](float ms)
	{ return size_t(sampleRate * ms / 1000.0f); };

	const std::vector<Run> runs = syncRuns(freq, syncFilterWindow);

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
		float acc = 0.0f;
		size_t n = 0;
		for (size_t i = a; i < b && i < freq.size(); ++i, ++n)
			acc += freq[i];
		return n ? acc / float(n) : 0.0f;
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

Decoder::Decoder(const std::string &output, uint32_t width,
                 std::unique_ptr<Demodulator> demod,
                 SimpleMovingAverage syncFilter)
	: width(width),
	  // Read the sample rate before moving demod (member init follows
	  // declaration order, so sampleRate is initialised before demod).
	  sampleRate(demod->sampleRate()),
	  demod(std::move(demod)),
	  output(output),
	  // Take the SMA the caller picked. Default = SimpleMovingAverage(1),
	  // the identity (window 1 → output equals input, delay 0). Ring buffer
	  // for delay compensation sized accordingly: empty for identity, real
	  // size when the caller passes a wider SMA. No branches in the hot
	  // path — polymorphism by data.
	  syncFilter(std::move(syncFilter)),
	  syncDelayBuf(this->syncFilter.delay(), 0.0f)
{
}

void Decoder::feed(std::span<const float> audio)
{
	if (finished)
		return;
	for (float sample : audio)
	{
		processFreq(globalIndex, demod->process(sample));
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

	VIS v = detectVIS(headerBuf, sampleRate, syncFilter.windowSize());
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
	// Update the delay ring buffer first, so a line-start triggered later
	// in this iteration can pre-pend the most recent `delay` raw samples.
	if (!syncDelayBuf.empty())
	{
		syncDelayBuf[syncDelayPos] = freq;
		if (++syncDelayPos >= syncDelayBuf.size())
			syncDelayPos = 0;
	}

	// Sync detection: optionally smooth the freq stream (when smooth-sync
	// is enabled syncFilter is a 1.25 ms boxcar, otherwise it's the
	// identity SMA(1)) then run through the Schmitt trigger. Silence
	// (freq <= 0) bypasses both — it can never be sync — but it also
	// doesn't update the smoother, so its state survives a silent gap.
	const bool sub = (freq > 0.0f) && syncTrigger.update(syncFilter.update(freq));

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
			prependSyncDelayToLineBuf();
		}
		else
		{
			// This sync bounds the previous line: [lineStart, runBegin).
			size_t contentLen = runBegin > lineStart ? runBegin - lineStart : 0;
			contentLen = std::min(contentLen, lineBuf.size());
			decodeLine(std::span<const float>(lineBuf.data(), contentLen));
			lineStart = index;
			lineBuf.clear();
			prependSyncDelayToLineBuf();
		}
	}
}

void Decoder::prependSyncDelayToLineBuf()
{
	// Walk the ring buffer in chronological order (oldest first) and
	// dump it into lineBuf. lineBuf[0] then corresponds to clock time
	// (current index - delay), compensating for the smoothing filter's
	// group delay: the resulting line content starts at the true sync
	// end rather than `delay` samples late.
	for (std::size_t k = 0; k < syncDelayBuf.size(); ++k)
	{
		std::size_t idx = (syncDelayPos + k) % syncDelayBuf.size();
		lineBuf.push_back(syncDelayBuf[idx]);
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

std::vector<float> Decoder::sampleChannel(std::span<const float> content,
										  size_t off, size_t span) const
{
	std::vector<float> row(width);
	for (uint32_t x = 0; x < width; ++x)
	{
		// Pixel x occupies an even slice of the channel's sample span.
		size_t a = off + span * x / width;
		size_t b = off + span * (x + 1) / width;
		if (b <= a)
			b = a + 1;

		float acc = 0.0f;
		size_t n = 0;
		for (size_t i = a; i < b && i < content.size(); ++i, ++n)
			acc += content[i];
		float f = n ? acc / float(n) : 0.0f;

		row[x] = std::clamp((f - sstv::Black) / (sstv::White - sstv::Black) * 255.0f,
							0.0f, 255.0f);
	}
	return row;
}
