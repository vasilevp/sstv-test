#include "bandpass_demodulator.hpp"

#include <utility>

BandpassDemodulator::BandpassDemodulator(std::unique_ptr<Demodulator> inner,
                                         float lowHz, float highHz)
	: inner_(std::move(inner))
{
	// HPF + LPF cascade, each a Butterworth biquad. Two second-order stages
	// give 40 dB/decade rolloff outside the passband — enough to crush
	// mains hum (~47 dB at 60 Hz) without disturbing the 1100..2300 Hz
	// SSTV tone range.
	const float sr = float(inner_->sampleRate());
	hpf_.setHighpass(lowHz, sr);
	lpf_.setLowpass(highHz, sr);
}

float BandpassDemodulator::process(float sample)
{
	return inner_->process(lpf_.process(hpf_.process(sample)));
}
