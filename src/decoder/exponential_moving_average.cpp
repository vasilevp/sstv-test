#include "exponential_moving_average.hpp"

#include <cmath>
#include <numbers>

ExponentialMovingAverage::ExponentialMovingAverage() = default;

float ExponentialMovingAverage::avg(float input)
{
	prev_ = prev_ * (1.0f - alpha_) + alpha_ * input;
	return prev_;
}

void ExponentialMovingAverage::reset()
{
	prev_ = 0.0f;
}

void ExponentialMovingAverage::setAlpha(float alpha)
{
	alpha_ = alpha;
}

void ExponentialMovingAverage::setCutoff(float freq, float rate, int order)
{
	// Closed form per xdsopl/robot36: solve for the α that puts a
	// first-order EMA's -3 dB point at `freq`, then take the (1/order)
	// root so cascading `order` such filters lands at the same cutoff.
	const float omega = 2.0f * std::numbers::pi_v<float> * freq / rate;
	const float x = std::cos(omega);
	const float a = x - 1.0f + std::sqrt(x * (x - 4.0f) + 3.0f);
	alpha_ = std::pow(a, 1.0f / float(order));
}
