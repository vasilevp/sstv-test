#include "biquad.hpp"

#include <cmath>
#include <numbers>

namespace
{
	constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
}

void Biquad::setLowpass(float cutoffHz, float sampleRate, float Q)
{
	const float omega = TwoPi * cutoffHz / sampleRate;
	const float cosOmega = std::cos(omega);
	const float sinOmega = std::sin(omega);
	const float alpha = sinOmega / (2.0f * Q);
	const float a0 = 1.0f + alpha;

	b0 = (1.0f - cosOmega) * 0.5f / a0;
	b1 = (1.0f - cosOmega) / a0;
	b2 = (1.0f - cosOmega) * 0.5f / a0;
	a1 = -2.0f * cosOmega / a0;
	a2 = (1.0f - alpha) / a0;
	z1 = 0.0f;
	z2 = 0.0f;
}

void Biquad::setHighpass(float cutoffHz, float sampleRate, float Q)
{
	const float omega = TwoPi * cutoffHz / sampleRate;
	const float cosOmega = std::cos(omega);
	const float sinOmega = std::sin(omega);
	const float alpha = sinOmega / (2.0f * Q);
	const float a0 = 1.0f + alpha;

	b0 = (1.0f + cosOmega) * 0.5f / a0;
	b1 = -(1.0f + cosOmega) / a0;
	b2 = (1.0f + cosOmega) * 0.5f / a0;
	a1 = -2.0f * cosOmega / a0;
	a2 = (1.0f - alpha) / a0;
	z1 = 0.0f;
	z2 = 0.0f;
}

void Biquad::setBandpass(float centerHz, float sampleRate, float Q)
{
	const float omega = TwoPi * centerHz / sampleRate;
	const float cosOmega = std::cos(omega);
	const float sinOmega = std::sin(omega);
	const float alpha = sinOmega / (2.0f * Q);
	const float a0 = 1.0f + alpha;

	b0 = alpha / a0;
	b1 = 0.0f;
	b2 = -alpha / a0;
	a1 = -2.0f * cosOmega / a0;
	a2 = (1.0f - alpha) / a0;
	z1 = 0.0f;
	z2 = 0.0f;
}

float Biquad::process(float x)
{
	const float y = b0 * x + z1;
	z1 = b1 * x - a1 * y + z2;
	z2 = b2 * x - a2 * y;
	return y;
}
