#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "demodulator.hpp"
#include "row_sink.hpp"
#include "schmitt_trigger.hpp"
#include "common/vis.hpp"
#include "vis_detector.hpp"

// Streaming base class for SSTV mode decoders.
//
// Audio is pushed in with feed() and decoded incrementally — the decoder
// never needs the whole signal in memory. Internally it runs a state machine:
// it demodulates each sample, accumulates the header until the VIS code is
// known, then tracks scanline sync pulses and hands each completed line's
// frequency samples to the mode-specific decodeLine(). finish() flushes the
// trailing line and writes the image, so a stream cut off mid-transmission
// still yields a partial picture.
class Decoder
{
public:
	virtual ~Decoder() = default;

	// Decoded header VIS (Vertical Interval Signalling) code: the value the
	// encoder writes to identify the SSTV mode.
	using VIS = sstv::Vis;

	// Push a block of normalised audio samples ([-1, 1)). May be called any
	// number of times; decoding happens incrementally as data arrives.
	void feed(std::span<const float> audio);

	// Signal end of stream: flush the final scanline and write the image.
	void finish();

	// Decode the VIS code from an already-demodulated frequency stream.
	// Returns found == false if the buffer does not yet cover a full
	// header.
	static VIS detectVIS(const std::vector<float> &freq, uint32_t sampleRate);

protected:
	// Constructor injection: the caller picks where decoded rows go, the
	// demodulator, and whether to engage cadence-locked sync acceptance
	// (default off — every Schmitt-detected sync edge ends a scanline).
	// The decoder takes ownership of `sink` and calls sink->finish() once
	// finish() runs, so any subclass-owned per-row routing (TFT, BMP file,
	// network socket) gets a clean shutdown signal.
	Decoder(std::unique_ptr<RowSink> sink, uint32_t width,
	        std::unique_ptr<Demodulator> demod,
	        bool cadenceLock = false);

	// Decode one scanline's content — every frequency sample between the end
	// of its sync pulse and the start of the next line's sync — into one or
	// more pixel rows, each handed back through emitRow().
	virtual void decodeLine(std::span<const float> content) = 0;

	// Nominal sample count of one line's content. Used only to bound the
	// final line, which has no following sync pulse to delimit it.
	virtual size_t nominalContentSamples() const = 0;

	// Nominal sample count of the full sync-to-sync line period (sync
	// pulse + content). The cadence validator uses this as the initial
	// estimate of the sync interval, then EMA-tracks any clock skew.
	virtual size_t nominalLinePeriodSamples() const = 0;

	// Append one decoded pixel row (width * 3 bytes, RGB) to the image.
	void emitRow(const std::vector<uint8_t> &rgb);

	// Sample `width` channel values (0..255) from the frequency span
	// content[off, off + span), inverting the encoder's value -> frequency
	// mapping. The shared per-pixel sampler for every mode's decodeLine().
	std::vector<float> sampleChannel(std::span<const float> content,
	                                 size_t off, size_t span) const;

	size_t ms2samp(float ms) const { return size_t(sampleRate * ms / 1000.0f); }

	uint32_t width;
	uint32_t sampleRate;
	VIS vis;

private:
	void processFreq(size_t index, float freq);
	void processHeader(size_t index, float freq);
	void processImage(size_t index, float freq);
	void recordSyncInterval(size_t observed);

	std::unique_ptr<Demodulator> demod;
	std::unique_ptr<RowSink> sink;

	enum class State
	{
		Header, // hunting for the calibration + VIS header
		Image,  // tracking scanline syncs and decoding lines
	} state = State::Header;

	// Header state: streaming VIS detector, fed one sample at a time.
	VisDetector visDetector;

	// Image state: incremental sync-pulse tracking and line accumulation.
	SchmittTrigger syncTrigger{sstv::SyncEnterHz, sstv::SyncExitHz};
	bool inRun = false;         // currently inside a sync-band run
	size_t runBegin = 0;        // start index of the current run
	bool haveLine = false;      // a scanline is being accumulated
	size_t lineStart = 0;       // index where the current line's content starts
	std::vector<float> lineBuf; // frequency samples of the current line

	// Adaptive sync-cadence tracking (xdsopl/robot36 v2 design). The scan-line
	// period is *measured* from the spacing of recent real sync pulses, never
	// assumed from the mode's nominal timing — so the decode follows the
	// transmitter's actual clock and the picture doesn't slant. A detected sync
	// is accepted when its interval sits near the measured period, re-aligning
	// the line to the real pulse; a much shorter interval is a spurious
	// mid-line blip and is ignored (the line keeps accumulating). When a sync
	// is overdue, a synthetic one is placed at the measured period so dropouts
	// are bridged at the correct pitch. `scanLinePeriod` is the mean of the
	// last few accepted intervals, committed only while they agree (low
	// standard deviation), which stops a stray observation poisoning the lock.
	bool cadenceLock = false;
	static constexpr size_t cadenceHistory = 5; // intervals averaged
	size_t syncIntervals[cadenceHistory] = {};  // recent sync-to-sync intervals
	size_t syncIntervalCount = 0;                // how many recorded so far
	size_t scanLinePeriod = 0;  // measured line period, samples (0 = not yet)
	size_t lastSyncBegin = 0;   // runBegin of the last accepted sync
	bool haveAnchor = false;    // lastSyncBegin has been initialised

	// Rows are forwarded to `sink` as they're decoded — no per-image buffer.
	// `imageHeight` is the running count, used for the "decoded N scanlines"
	// summary in finish() and bounded only by the recording length.
	uint32_t imageHeight = 0;

	size_t globalIndex = 0; // running count of demodulated samples
	bool finished = false;
};
