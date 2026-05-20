#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Turns an SSTV audio recording into a per-sample instantaneous-frequency
// signal using zero-crossing detection.
//
// This is the inverse of the encoder's Synthesizer: where the encoder maps a
// frequency to a stream of samples, the Demodulator recovers the frequency
// each sample was carrying. Zero-crossing demodulation is cheap and accurate
// enough for clean, synthetic recordings such as this project's own encoder
// output; noisy off-air captures would want a proper quadrature discriminator.
class Demodulator
{
	uint32_t sample_rate_ = 0;
	std::vector<float> freq_; // instantaneous frequency (Hz), one entry per sample

public:
	explicit Demodulator(const std::string &wavPath);

	uint32_t sampleRate() const { return sample_rate_; }
	size_t size() const { return freq_.size(); }

	// Time/sample conversions.
	size_t ms2samp(float ms) const { return size_t(sample_rate_ * ms / 1000.0f); }
	float samp2ms(size_t samples) const { return 1000.0f * samples / float(sample_rate_); }

	// Instantaneous frequency at a sample index; 0 Hz outside the signal.
	float at(size_t sample) const
	{
		return sample < freq_.size() ? freq_[sample] : 0.0f;
	}

	// Mean frequency over the half-open sample range [begin, end).
	float average(size_t begin, size_t end) const
	{
		end = std::min(end, freq_.size());
		if (begin >= end)
			return 0.0f;
		double acc = 0.0;
		for (size_t i = begin; i < end; ++i)
			acc += freq_[i];
		return float(acc / double(end - begin));
	}
};
