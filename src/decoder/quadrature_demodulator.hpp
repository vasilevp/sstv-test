#pragma once
#include <cstddef>
#include <cstdint>

#include "demodulator.hpp"
#include "kaiser_fir.hpp"

// Streaming complex-baseband FM discriminator — the "real" SSTV demodulator
// design, as used by QSSTV/MMSSTV and standard FM-demod literature.
//
// Each audio sample is multiplied by a numerically-controlled oscillator
// running at a fixed centre frequency to produce a complex baseband signal
// (I, Q). A pair of two-pole IIR low-pass filters strips the sum-frequency
// component, leaving the analytic signal around DC. The instantaneous
// frequency is then the time derivative of the unwrapped phase, computed
// cheaply as arg(z[n] * conj(z[n-1])).
//
// Compared to zero-crossing demodulation: noise is integrated over the LPF's
// narrow bandwidth (so off-air recordings with poor SNR demodulate gracefully
// instead of catastrophically), and the frequency estimate updates every
// sample rather than only at zero crossings.
class QuadratureDemodulator : public Demodulator
{
public:
	explicit QuadratureDemodulator(uint32_t sampleRate);

	float process(float sample) override;
	uint32_t sampleRate() const override { return rate_; }

private:
	// Mid-band of the SSTV signal (mid-point of 1100..2300 Hz). Choosing
	// this centre keeps the baseband signal symmetric around DC and lets a
	// narrow LPF cover the whole tone range.
	static constexpr float CenterHz = 1700.0f;

	// LPF passband edge: comfortably above the ±600 Hz baseband swing,
	// far below the sum-frequency component at 2·CenterHz = 3400 Hz.
	static constexpr float LpfCutoffHz = 800.0f;

	// Width of the FIR's transition band. Stopband edge therefore lands
	// at 800 + 900 = 1700 Hz — still ~1.7 kHz clear of the sum frequency.
	static constexpr float LpfTransitionHz = 900.0f;

	// Stopband attenuation. 50 dB is the standard headroom for audio-band
	// LPFs and roughly matches the dynamic range of 16-bit PCM input.
	static constexpr float LpfStopbandDb = 50.0f;

	uint32_t rate_;

	// NCO state: phase wraps in [0, 2π); incremented by phaseInc_ per sample.
	float phase_ = 0.0f;
	float phaseInc_;

	// Baseband filters for the in-phase and quadrature branches.
	// Linear-phase FIRs — the two branches share an identical group delay,
	// so the I/Q discriminator stays phase-coherent.
	KaiserFir lpfI_;
	KaiserFir lpfQ_;

	// Previous baseband I/Q, for the phase-derivative computation.
	float prevI_ = 0.0f;
	float prevQ_ = 0.0f;
	bool havePrev_ = false;

	float freq_ = 0.0f;
};
