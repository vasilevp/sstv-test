#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "demodulator.hpp"

// SSTV tone frequencies (Hz) — the decoding-side mirror of the encoder's
// Synthesizer::Frequency enum.
namespace sstv
{
	constexpr float Black = 1500.0f;	 // pixel luma 0
	constexpr float White = 2300.0f;	 // pixel luma 255
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

// Base class for SSTV mode decoders. Mirrors the encoder's Encoder: it owns
// the signal source (a Demodulator instead of a Synthesizer) and leaves the
// mode-specific scanline layout to Decode() in the derived class.
class Decoder
{
public:
	virtual ~Decoder() = default;

	virtual void Decode() = 0;

	// Decoded header VIS (Vertical Interval Signalling) code: the value the
	// encoder writes to identify the SSTV mode.
	struct VIS
	{
		bool found = false;	   // the header VIS section was located
		uint8_t code = 0;	   // 7-bit mode code
		bool parityOK = false; // decoded parity bit matched the code
		size_t headerEnd = 0;  // sample index just past the VIS stop marker
	};

	// Locate and decode the header VIS code. Static so callers can probe a
	// recording's mode before committing to a concrete decoder.
	static VIS detectVIS(const Demodulator &demod);

protected:
	Decoder(const std::string &input, const std::string &output)
		: demod(input), output(output)
	{
	}

	// A contiguous run of samples carrying a sync-band tone (< SyncThreshold).
	struct Run
	{
		size_t begin, end; // half-open sample range [begin, end)
	};

	// Every sync-band run in the recording, in order of appearance.
	static std::vector<Run> syncRuns(const Demodulator &demod);

	Demodulator demod;
	std::string output;
};
