#include "decoder.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <print>
#include <stdexcept>
#include <string>

#include "exponential_moving_average.hpp"
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
	return std::size_t(SyncFilterMs * sampleRate / 1000.0f) | 1u;
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
				 SimpleMovingAverage syncFilter,
				 bool cadenceLock)
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
	  syncDelayBuf(this->syncFilter.delay(), 0.0f),
	  cadenceLock(cadenceLock)
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

	// Cadence-locked synthesis: if a real sync was due by now and didn't
	// arrive, place a synthetic one at the predicted position. Keeps the
	// picture geometrically aligned through full-line dropouts. The
	// `+ cadenceWindow` past predictedEnd is what makes the synthesis
	// "late" enough that lineBuf contains the entire predicted line plus
	// the entire predicted sync — so we can split it cleanly.
	const size_t cadenceWindow = ms2samp(3.0f);
	if (cadenceLock && haveAnchor && haveLine && !inRun)
	{
		const size_t syncSamples = nominalLinePeriodSamples() > nominalContentSamples()
			? nominalLinePeriodSamples() - nominalContentSamples()
			: 0;
		const size_t predictedBegin = lastSyncBegin + expectedPeriod;
		const size_t predictedEnd = predictedBegin + syncSamples;
		if (index > predictedEnd + cadenceWindow)
		{
			// Decode the line content up to the predicted sync boundary.
			size_t contentLen = predictedBegin > lineStart ? predictedBegin - lineStart : 0;
			contentLen = std::min(contentLen, lineBuf.size());
			decodeLine(std::span<const float>(lineBuf.data(), contentLen));
			// Drop everything through the synthetic sync; keep whatever
			// has been accumulated since predictedEnd as the start of the
			// next line.
			size_t drop = predictedEnd > lineStart ? predictedEnd - lineStart : 0;
			drop = std::min(drop, lineBuf.size());
			lineBuf.erase(lineBuf.begin(), lineBuf.begin() + drop);
			lineStart = predictedEnd;
			lastSyncBegin = predictedBegin;
			// expectedPeriod unchanged — no observation to learn from.
		}
	}

	// Track sync-band runs. Past the header, every such run is normally one
	// scanline sync pulse — except in noisy data where Schmitt blips can
	// fire mid-line. Cadence validation filters those out.
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
			// First sync after the header: bootstrap. Line 0's content
			// starts at this sync's end; the cadence anchor is its begin.
			haveLine = true;
			lineStart = index;
			lineBuf.clear();
			prependSyncDelayToLineBuf();
			if (cadenceLock)
			{
				lastSyncBegin = runBegin;
				expectedPeriod = nominalLinePeriodSamples();
				haveAnchor = true;
				cadenceWarmup = true;
			}
		}
		else if (!cadenceLock)
		{
			// Original behaviour: every detected sync ends a line.
			size_t contentLen = runBegin > lineStart ? runBegin - lineStart : 0;
			contentLen = std::min(contentLen, lineBuf.size());
			decodeLine(std::span<const float>(lineBuf.data(), contentLen));
			lineStart = index;
			lineBuf.clear();
			prependSyncDelayToLineBuf();
		}
		else
		{
			// Cadence validation: accept iff this sync's start is within
			// ±cadenceWindow of the predicted position, or it's the first
			// post-bootstrap sync (warmup — Scottie's segment 0 is shorter
			// than nominal). EMA-update the period estimate on accept so
			// slow clock skew gets tracked, but skip the update during
			// warmup since segment 0's interval is structurally off.
			const size_t predictedBegin = lastSyncBegin + expectedPeriod;
			const bool inWindow =
				runBegin + cadenceWindow >= predictedBegin &&
				runBegin <= predictedBegin + cadenceWindow;
			if (cadenceWarmup || inWindow)
			{
				size_t contentLen = runBegin > lineStart ? runBegin - lineStart : 0;
				contentLen = std::min(contentLen, lineBuf.size());
				decodeLine(std::span<const float>(lineBuf.data(), contentLen));
				lineStart = index;
				lineBuf.clear();
				prependSyncDelayToLineBuf();
				if (!cadenceWarmup)
				{
					// EMA, α = 1/4: fast enough to follow a few-ppm clock
					// skew but slow enough that a single noisy observation
					// can't whipsaw the lock.
					const size_t observed = runBegin - lastSyncBegin;
					expectedPeriod = (3 * expectedPeriod + observed) / 4;
				}
				cadenceWarmup = false;
				lastSyncBegin = runBegin;
			}
			// Out-of-window sync: reject as spurious. The line keeps
			// accumulating; the sync-band samples added to lineBuf
			// during this run become a small dark notch in the decoded
			// row, which is far less harmful than ending the line early.
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
	// When the channel is heavily over-sampled (many audio samples per
	// pixel), per-slot box averaging behaves like a wide low-pass that
	// throws away in-pixel detail — and the line content has plenty of
	// frequency headroom above the pixel band that wants anti-aliasing.
	// In that regime, replace the box average with a zero-phase EMA
	// pre-smooth (forward + reverse first-order IIR, cutoff at pixel
	// Nyquist) followed by point sampling at slot centres. xdsopl/robot36
	// uses this design for the PD modes, where it's most clearly a win.
	//
	// For the under-sampled regime (few samples per pixel — Robot 36 Y,
	// Martin M2/M4, etc.), the same trick costs more than it gains: the
	// EMA does little because pixel-Nyquist sits near sample-Nyquist, and
	// point sampling loses the sqrt(N) noise reduction the box mean was
	// giving. Stay with the box mean there.
	const float samplesPerPixel = width > 0 ? float(span) / float(width) : 0.0f;
	const bool antiAlias = samplesPerPixel > 4.0f;

	std::vector<float> row(width);

	if (antiAlias)
	{
		// Lift the channel into a contiguous working buffer (the EMA can't
		// pass back over a span<const>), zero-padding any tail that runs
		// off the end of `content`.
		std::vector<float> work(span, 0.0f);
		const size_t copy = std::min(span, content.size() > off ? content.size() - off : 0);
		for (size_t i = 0; i < copy; ++i)
			work[i] = content[off + i];

		ExponentialMovingAverage ema;
		ema.setCutoff(float(width), float(2 * span), 2);
		for (size_t i = 0; i < span; ++i)
			work[i] = ema.avg(work[i]);
		ema.reset();
		for (size_t i = span; i-- > 0;)
			work[i] = ema.avg(work[i]);

		for (uint32_t x = 0; x < width; ++x)
		{
			size_t centre = (span * (2u * x + 1u)) / (2u * width);
			if (centre >= span)
				centre = span - 1;
			const float f = work[centre];
			row[x] = std::clamp((f - sstv::Black) / (sstv::White - sstv::Black) * 255.0f,
								0.0f, 255.0f);
		}
	}
	else
	{
		// Box-average per pixel slot — preserves noise reduction on the
		// closely-sampled fast modes.
		for (uint32_t x = 0; x < width; ++x)
		{
			size_t a = off + span * x / width;
			size_t b = off + span * (x + 1) / width;
			if (b <= a)
				b = a + 1;

			float acc = 0.0f;
			size_t n = 0;
			for (size_t i = a; i < b && i < content.size(); ++i, ++n)
				acc += content[i];
			const float f = n ? acc / float(n) : 0.0f;
			row[x] = std::clamp((f - sstv::Black) / (sstv::White - sstv::Black) * 255.0f,
								0.0f, 255.0f);
		}
	}

	return row;
}
