#include "quadrature_demodulator.hpp"

#include <cmath>
#include <numbers>

namespace
{
	constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
}

void QuadratureDemodulator::Biquad::setLowpass(float cutoffHz, float sampleRate)
{
	// Audio EQ Cookbook (Robert Bristow-Johnson): biquad low-pass with
	// Q = 1/sqrt(2), the maximally-flat Butterworth shape.
	const float omega = TwoPi * cutoffHz / sampleRate;
	const float cosOmega = std::cos(omega);
	const float sinOmega = std::sin(omega);
	const float alpha = sinOmega / std::sqrt(2.0f);
	const float a0 = 1.0f + alpha;

	b0 = (1.0f - cosOmega) * 0.5f / a0;
	b1 = (1.0f - cosOmega) / a0;
	b2 = (1.0f - cosOmega) * 0.5f / a0;
	a1 = -2.0f * cosOmega / a0;
	a2 = (1.0f - alpha) / a0;
	z1 = 0.0f;
	z2 = 0.0f;
}

float QuadratureDemodulator::Biquad::process(float x)
{
	// Transposed direct form II: one multiply-add for the output, two state
	// updates. Numerically robust and well-suited to single-precision float.
	const float y = b0 * x + z1;
	z1 = b1 * x - a1 * y + z2;
	z2 = b2 * x - a2 * y;
	return y;
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
