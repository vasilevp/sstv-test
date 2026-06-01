#pragma once

// First-order IIR exponential moving average: y[n] = α·x[n] + (1-α)·y[n-1].
// Cheap state (one float), zero allocation, useful as a building block for
// low-pass filtering — and especially useful as the kernel of a forward +
// reverse two-pass IIR (call avg() across the buffer, reset(), then avg()
// again in reverse), which cancels out the EMA's group delay and gives a
// zero-phase low-pass response.
//
// Modelled on ExponentialMovingAverage in xdsopl/robot36 (Inan, 2024).
class ExponentialMovingAverage
{
public:
	ExponentialMovingAverage();

	// Apply one filter step.
	float avg(float input);

	// Reset state to zero (call between forward and reverse passes).
	void reset();

	// Set α directly.
	void setAlpha(float alpha);

	// Pick α so cascading the EMA `order` times (e.g. forward + reverse =
	// order 2) lands its -3 dB point at `freq` (in the same units as `rate`).
	// Closed-form solution from xdsopl/robot36.
	void setCutoff(float freq, float rate, int order = 1);

private:
	float alpha_ = 1.0f;
	float prev_ = 0.0f;
};
