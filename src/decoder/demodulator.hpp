#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

// Abstract streaming FM demodulator: produces one instantaneous-frequency
// estimate per audio sample fed to it. Concrete implementations sit behind
// this interface so the decoder can swap algorithms without code changes.
class Demodulator
{
public:
	virtual ~Demodulator() = default;

	// Process one normalised audio sample; returns its instantaneous
	// frequency in Hz (0 until the implementation has settled).
	virtual float process(float sample) = 0;

	virtual uint32_t sampleRate() const = 0;
};

// Which demodulation algorithm to use.
enum class DemodKind
{
	// Zero-crossing half-period measurement. Fast, drift-free,
	// pixel-accurate on clean synthetic signals; degrades catastrophically
	// in the presence of noise.
	ZeroCrossing,

	// Complex-baseband FM discriminator (NCO mixer + low-pass + phase
	// derivative). The standard "real" SSTV demodulator: integrates noise
	// over a narrow baseband filter and updates the frequency estimate
	// every sample instead of every zero crossing.
	Quadrature,
};

// Human-readable name for a demodulator kind ("zero-crossing" or
// "quadrature"). Used by the CLI to echo back the user's choice.
const char *demodKindName(DemodKind kind);

// Build a fresh demodulator of the requested kind.
std::unique_ptr<Demodulator> makeDemodulator(DemodKind kind, uint32_t sampleRate);
