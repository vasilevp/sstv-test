#pragma once
#include <cstddef>
#include <cstdint>

// Streaming zero-crossing FM demodulator: the inverse of the encoder's
// Synthesizer, producing one instantaneous-frequency estimate per audio
// sample fed to it.
//
// Each completed half-cycle (the span between two zero crossings) implies a
// frequency; that value is held until the next crossing refreshes it. This is
// causal — a sample carries the frequency of the half-cycle just before it —
// so the demodulator needs no look-ahead and keeps only a few values of
// state, which is what makes streaming decoding possible.
class Demodulator
{
	uint32_t rate_;
	float prev_ = 0.0f;          // previous input sample
	size_t index_ = 0;           // running count of samples processed
	double lastCrossing_ = 0.0;  // sample position of the last zero crossing
	bool haveCrossing_ = false;  // a crossing has been seen
	float freq_ = 0.0f;          // frequency of the most recent half-cycle

public:
	explicit Demodulator(uint32_t sampleRate) : rate_(sampleRate) {}

	uint32_t sampleRate() const { return rate_; }

	// Process one normalised audio sample; returns its instantaneous
	// frequency in Hz (0 until the first half-cycle completes).
	float process(float sample)
	{
		if ((prev_ < 0.0f && sample >= 0.0f) || (prev_ > 0.0f && sample <= 0.0f))
		{
			// Linearly interpolate the sub-sample crossing position.
			double frac = double(prev_) / (double(prev_) - double(sample));
			double crossing = double(index_) - 1.0 + frac;
			if (haveCrossing_)
			{
				double half = crossing - lastCrossing_;
				if (half > 0.0)
					freq_ = float(0.5 * double(rate_) / half);
			}
			lastCrossing_ = crossing;
			haveCrossing_ = true;
		}
		prev_ = sample;
		++index_;
		return freq_;
	}
};
