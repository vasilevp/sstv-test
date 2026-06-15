#include "decoder.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <print>
#include <stdexcept>
#include <string>

#include "exponential_moving_average.hpp"

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

uint32_t sstv::visModeWidth(uint8_t code)
{
	// Widths are the "Columns" field of the mode table in the SSTV Handbook
	// (Bruchanov OK2MNM), chapter 5 "List of SSTV modes":
	//   https://www.sstv-handbook.com/download/sstv_05.pdf
	switch (code)
	{
	// Robot B&W 8 (one VIS per R/G/B filter component). The handbook's row
	// is internally transposed (it prints Lines=160/Columns=120, but 900 lpm
	// over 8 s is 120 lines); the consistent, canonical frame is 160x120.
	case 1:
	case 2:
	case 3:
		return 160;
	// PD family. The high-res variants are the reason this table exists:
	// without it a PD120 frame decodes at the 320 default and comes out
	// stretched 2:1 vertically (the row count is fixed by the transmission).
	case 94: // PD 290
		return 800;
	case 98: // PD 160
		return 512;
	case 95: // PD 120
	case 96: // PD 180
	case 97: // PD 240
		return 640;
	// Everything else this decoder builds — Robot 36/72, Martin M1..M4,
	// Scottie S1..S4/DX, PD 50/90 — is the classic 320-wide grid.
	default:
		return 320;
	}
}

Decoder::VIS Decoder::detectVIS(const std::vector<float> &freq, uint32_t sampleRate)
{
	// Replay the buffer through the streaming detector in one pass; the same
	// state machine that runs live drives this offline probe. Returns the
	// first lock, or a not-found result if the buffer holds no header.
	VisDetector det(sampleRate);
	for (float f : freq)
		if (det.update(f))
			break;
	return det.result();
}

Decoder::Decoder(std::unique_ptr<RowSink> sink, uint32_t width,
				 std::unique_ptr<Demodulator> demod,
				 bool cadenceLock)
	: width(width),
	  // Read the sample rate before moving demod (member init follows
	  // declaration order, so sampleRate is initialised before demod).
	  sampleRate(demod->sampleRate()),
	  demod(std::move(demod)),
	  sink(std::move(sink)),
	  visDetector(sampleRate),
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
	// Feed the sample to the streaming detector; it keeps its own state, so
	// this is O(1) per sample with no rescanning. No header-search timeout
	// here: this is a streaming decoder, so how long to wait for a header is
	// the caller's policy. If the stream ends while still hunting, finish()
	// reports the failure.
	if (!visDetector.update(freq))
		return;

	// Lock in the mode and switch to image decoding. The detector locks on
	// the sample just past the VIS stop marker, so this very sample is the
	// first one of the image — hand it straight to processImage().
	vis = visDetector.result();
	std::println("VIS code: {} ({}){}", int(vis.code), sstv::visModeName(vis.code),
				 vis.parityOK ? "" : "  [PARITY MISMATCH]");
	if (!vis.parityOK)
		std::println("  warning: VIS parity check failed; the recording may be corrupt");
	state = State::Image;
	processImage(index, freq);
}

void Decoder::processImage(size_t index, float freq)
{
	// Sync detection: feed the raw freq sample to the Schmitt trigger.
	// Silence (freq <= 0) bypasses it — silence can never be sync.
	const bool sub = (freq > 0.0f) && syncTrigger.update(freq);

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

	// Push the EOS signal to the sink — BMP files patch their header here;
	// embedded sinks (TFT, SD card) might flush a DMA queue.
	sink->finish();
}

void Decoder::emitRow(const std::vector<uint8_t> &rgb)
{
	sink->row(std::span<const std::uint8_t>(rgb.data(), rgb.size()));
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
