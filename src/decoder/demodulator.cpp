#include "demodulator.hpp"

Demodulator::Demodulator(uint32_t sampleRate) : rate_(sampleRate)
{
}

float Demodulator::process(float sample)
{
	if ((prev_ < 0.0f && sample >= 0.0f) || (prev_ > 0.0f && sample <= 0.0f))
	{
		// The zero crossing lies between samples index_-1 and index_; its
		// position is that integer index plus a sub-sample fraction.
		size_t crossIndex = index_ - 1;
		float crossFrac = prev_ / (prev_ - sample); // in [0, 1]

		if (haveCrossing_)
		{
			// Half-period between this crossing and the previous one. The
			// integer sample span is computed in exact integer arithmetic;
			// only the small fractional correction (in [-1, 1]) needs
			// floating point. So the estimate stays precise however long the
			// stream runs — unlike accumulating an ever-growing absolute
			// crossing position in a float, whose 24-bit mantissa would lose
			// sub-sample accuracy after a few minutes of audio.
			float half = float(crossIndex - lastCrossIndex_)
			             + (crossFrac - lastCrossFrac_);
			if (half > 0.0f)
				freq_ = 0.5f * float(rate_) / half;
		}
		lastCrossIndex_ = crossIndex;
		lastCrossFrac_ = crossFrac;
		haveCrossing_ = true;
	}
	prev_ = sample;
	++index_;
	return freq_;
}
