#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "demodulator.hpp"

// SSTV tone frequencies (Hz) — the decoding-side mirror of the encoder's
// Synthesizer::Frequency enum.
namespace sstv
{
	constexpr float Black = 1500.0f;     // pixel luma 0
	constexpr float White = 2300.0f;     // pixel luma 255
	constexpr float SyncPulse = 1200.0f; // scanline / VIS sync tone

	constexpr float VISOne = 1100.0f;  // a VIS data bit valued 1
	constexpr float VISZero = 1300.0f; // a VIS data bit valued 0

	// A tone below this is treated as sync rather than pixel data. It sits
	// between SyncPulse (1200) and VISZero (1300) so VIS data bits are not
	// mistaken for sync pulses, while pixel tones (>= Black) stay well clear.
	constexpr float SyncThreshold = 1280.0f;

	// Human-readable name for a VIS mode code, or "unknown".
	const char *visModeName(uint8_t code);
}

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
	struct VIS
	{
		bool found = false;    // the header VIS section was located
		uint8_t code = 0;      // 7-bit mode code
		bool parityOK = false; // decoded parity bit matched the code
		size_t headerEnd = 0;  // sample index just past the VIS stop marker
	};

	// Push a block of normalised audio samples ([-1, 1)). May be called any
	// number of times; decoding happens incrementally as data arrives.
	void feed(std::span<const float> audio);

	// Signal end of stream: flush the final scanline and write the image.
	void finish();

	// Decode the VIS code from an already-demodulated frequency stream.
	// Returns found == false if the buffer does not yet cover a full header.
	static VIS detectVIS(const std::vector<float> &freq, uint32_t sampleRate);

protected:
	Decoder(const std::string &output, uint32_t width, uint32_t sampleRate);

	// Decode one scanline's content — every frequency sample between the end
	// of its sync pulse and the start of the next line's sync — into one or
	// more pixel rows, each handed back through emitRow().
	virtual void decodeLine(std::span<const float> content) = 0;

	// Nominal sample count of one line's content. Used only to bound the
	// final line, which has no following sync pulse to delimit it.
	virtual size_t nominalContentSamples() const = 0;

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

	Demodulator demod;
	std::string output;

	enum class State
	{
		Header, // accumulating the calibration + VIS header
		Image,  // tracking scanline syncs and decoding lines
	} state = State::Header;

	// Header state: frequency samples buffered until the VIS code is read.
	std::vector<float> headerBuf;

	// Image state: incremental sync-pulse tracking and line accumulation.
	bool inRun = false;         // currently inside a sync-band run
	size_t runBegin = 0;        // start index of the current run
	bool haveLine = false;      // a scanline is being accumulated
	size_t lineStart = 0;       // index where the current line's content starts
	std::vector<float> lineBuf; // frequency samples of the current line

	std::vector<uint8_t> image; // decoded RGB rows, row-major
	uint32_t imageHeight = 0;

	size_t globalIndex = 0; // running count of demodulated samples
	bool finished = false;
};
