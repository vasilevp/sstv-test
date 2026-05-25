#include "quadrature_demodulator.hpp"

#include <cmath>
#include <numbers>

namespace
{
	constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
}

QuadratureDemodulator::QuadratureDemodulator(uint32_t sampleRate)
	: rate_(sampleRate),
	  phaseInc_(TwoPi * CenterHz / float(sampleRate))
{
	lpfI_.setLowpass(LpfCutoffHz, float(sampleRate));
	lpfQ_.setLowpass(LpfCutoffHz, float(sampleRate));
}

float QuadratureDemodulator::process(float sample)
{
	// Down-convert: multiply the real input by exp(-j * 2π * fc * t) to
	// shift the signal centred at fc down to baseband.
	const float c = std::cos(phase_);
	const float s = std::sin(phase_);
	const float rawI = sample * c;
	const float rawQ = -sample * s;

	phase_ += phaseInc_;
	if (phase_ >= TwoPi)
		phase_ -= TwoPi;

	// Strip the sum-frequency component, leaving the analytic signal
	// around DC.
	const float i = lpfI_.process(rawI);
	const float q = lpfQ_.process(rawQ);

	if (havePrev_)
	{
		// Phase difference between consecutive samples:
		//   arg(z[n] * conj(z[n-1]))
		// = atan2(q*prevI - i*prevQ,  i*prevI + q*prevQ)
		// which is exactly the instantaneous baseband frequency in
		// radians per sample. Add it back to the carrier to get Hz.
		const float dx = i * prevI_ + q * prevQ_;
		const float dy = q * prevI_ - i * prevQ_;
		const float dphi = std::atan2(dy, dx);
		freq_ = CenterHz + dphi * float(rate_) / TwoPi;
	}
	else
	{
		havePrev_ = true;
	}
	prevI_ = i;
	prevQ_ = q;
	return freq_;
}
