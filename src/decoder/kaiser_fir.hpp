#pragma once
#include <cstddef>
#include <vector>

// Linear-phase low-pass FIR designed by the Kaiser window method.
//
// The Kaiser window is the textbook choice when you want to dial in a
// specific stopband attenuation against a budget of taps: β is chosen to
// hit the requested rejection (Oppenheim & Schafer eq. 10.13), and the
// required filter length follows from the transition width
// (Kaiser/Schafer estimate, A = (N-8)·2.285·Δω / 2π).
//
// Compared to the Butterworth biquad the IQ demodulator used to use, the
// FIR's roll-off is dramatically steeper (≈50 dB by the stopband edge
// vs. ≈12 dB/octave forever) and its phase response is linear — so the
// I and Q branches share an identical group delay, which keeps the
// arg(z[n]·conj(z[n-1])) phase-discriminator clean. The cost is the tap
// memory and N multiply-adds per sample, both modest at typical SSTV
// audio rates.
class KaiserFir
{
public:
	KaiserFir();

	// Design a low-pass with the given passband edge and transition width
	// (Hz). The stopband edge therefore lands at cutoffHz + transitionHz;
	// the stopband floor is at least stopbandAttenDb down. Re-callable;
	// resets state.
	void setLowpass(float cutoffHz, float transitionHz, float sampleRate,
	                float stopbandAttenDb = 50.0f);

	float process(float input);
	void reset();

	std::size_t tapCount() const { return taps_.size(); }

private:
	std::vector<float> taps_;
	// Delay line stored twice back-to-back (length 2N): writing each new
	// sample to both hist_[pos_] and hist_[pos_ + N] keeps the N most recent
	// samples available as one *contiguous* run, so process() can convolve
	// with a flat dot product the compiler can vectorise — no per-tap modulo
	// wrap, no branch.
	std::vector<float> hist_;
	std::size_t pos_ = 0;
};
