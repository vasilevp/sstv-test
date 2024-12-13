#pragma once
#include <stdint.h>
#include <cmath>

#include "utils.hpp"
#include "wav.hpp"

class Synthesizer
{
public:
	Synthesizer(const std::string &output, int sample_rate = 8000)
		: sample_rate(sample_rate),
		  freq_step(sample_rate / utils::lut.size()),
		  w(output, sample_rate)
	{
	}

	constexpr void synth(float length, size_t freq)
	{
		frame += ms2samp(length, sample_rate);
		int newframe = frame;
		frame -= newframe;
		while (newframe-- > 0)
		{
			uint8_t x = utils::lut[((idx / freq_step) % utils::lut.size())] + 128;
			idx += freq;
			w.put(x);
		}
	}

private:
	WAVWriter w;
	// MUST BE A MULTIPLE OF 2000
	const size_t sample_rate;
	const int freq_step;

	float frame = 0;
	int idx = 0;
	static constexpr float ms2samp(float ms, size_t sample_rate)
	{
		return sample_rate * ms / 1000;
	}
};