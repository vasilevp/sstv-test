#pragma once
#include <string>

#include "demodulator.hpp"

// SSTV tone frequencies (Hz) — the decoding-side mirror of the encoder's
// Synthesizer::Frequency enum.
namespace sstv
{
	constexpr float Black = 1500.0f;     // pixel luma 0
	constexpr float White = 2300.0f;     // pixel luma 255
	constexpr float SyncPulse = 1200.0f; // scanline / VIS sync tone

	// A tone below this is treated as sync rather than pixel data. It sits
	// between SyncPulse (1200) and VISZero (1300) so VIS data bits are not
	// mistaken for sync pulses, while pixel tones (>= Black) stay well clear.
	constexpr float SyncThreshold = 1280.0f;
}

// Base class for SSTV mode decoders. Mirrors the encoder's Encoder: it owns
// the signal source (a Demodulator instead of a Synthesizer) and leaves the
// mode-specific scanline layout to Decode() in the derived class.
class Decoder
{
public:
	virtual ~Decoder() = default;

	virtual void Decode() = 0;

protected:
	Decoder(const std::string &input, const std::string &output)
		: demod(input), output(output)
	{
	}

	Demodulator demod;
	std::string output;
};
