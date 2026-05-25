#pragma once
#include <cstddef>
#include <vector>

// Sliding-window mean over the last N samples — a fixed-tap boxcar FIR.
// Constant per-sample cost (one add, one subtract, one divide); ring-buffer
// state. Used in the decoder as the pre-trigger smoothing stage on the
// demodulator's frequency stream: short in-band noise spikes that would
// otherwise toggle the Schmitt trigger get integrated away over the
// window before threshold detection sees them.
//
// Matches the SimpleMovingAverage in xdsopl/robot36 (Inan, 2024). Output
// during the initial warm-up (first N samples) is divided by the same N
// rather than the running fill count, so the output ramps up from 0 to
// the steady-state value over one window — intentional, matches xdsopl.
class SimpleMovingAverage
{
public:
	explicit SimpleMovingAverage(std::size_t windowSize);

	// Push a sample, return the window mean.
	float update(float sample);

	void reset();

	std::size_t windowSize() const { return window_.size(); }

	// Group delay introduced by the window — useful if a caller wants to
	// correct event-position reports to true clock time.
	std::size_t delay() const { return (window_.size() - 1) / 2; }

private:
	std::vector<float> window_;
	std::size_t pos_ = 0;
	float sum_ = 0.0f;
};
