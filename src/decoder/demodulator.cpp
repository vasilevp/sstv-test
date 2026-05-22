#include "demodulator.hpp"

Demodulator::Demodulator(uint32_t sampleRate) : rate_(sampleRate)
{
}

float Demodulator::process(float sample)
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
