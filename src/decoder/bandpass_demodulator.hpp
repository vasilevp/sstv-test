#pragma once
#include <cstdint>
#include <memory>

#include "biquad.hpp"
#include "demodulator.hpp"

// Decorator that runs every audio sample through a band-limiting cascade
// (Butterworth highpass + lowpass biquads) before passing it to the wrapped
// Demodulator. Intended for noisy off-air recordings: drops mains hum at
// ~50/60 Hz hard (HPF rolls off at 40 dB/decade) and HF hiss above the SSTV
// band, while leaving the 1100..2300 Hz tone range alone.
//
// On the project's clean synthetic recordings this is a near-no-op (the
// passband swallows the whole signal); on a real radio capture it lets a
// downstream zero-crossing demodulator survive what would otherwise be
// catastrophic out-of-band noise.
class BandpassDemodulator : public Demodulator
{
public:
	// Defaults span 900..2500 Hz — a comfortable cushion around the SSTV
	// signal range of 1100..2300 Hz that still gives ~47 dB attenuation
	// at 60 Hz.
	BandpassDemodulator(std::unique_ptr<Demodulator> inner,
	                    float lowHz = 900.0f,
	                    float highHz = 2500.0f);

	float process(float sample) override;
	uint32_t sampleRate() const override { return inner_->sampleRate(); }

private:
	std::unique_ptr<Demodulator> inner_;
	Biquad hpf_;
	Biquad lpf_;
};
