#pragma once
#include <stdint.h>
#include <cmath>

#include "utils.hpp"
#include "wav.hpp"

enum Frequency : uint16_t
{
	Black = 1500,
	White = 2300,
	Grey = (Black + White) / 2,

	SyncPulse = 1200,
	SyncPorch = 1500,

	VISGrey = 1900,
	VISOne = 1100,
	VISZero = 1300,
};

class Synthesizer
{
public:
	Synthesizer(const std::string &output, int sample_rate = 8000)
		: sample_rate(sample_rate),
		  freq_step(sample_rate / utils::lut.size()),
		  w(output, sample_rate) {};

	Synthesizer(const char output[], int sample_rate = 8000) : Synthesizer(std::string(output), sample_rate) {};

	static inline Frequency Lerp(Frequency from, Frequency to, float f)
	{
		return Frequency(from + Frequency(float(to - from) * f));
	}

	static inline Frequency Lerp(float f)
	{
		return Lerp(Black, White, f);
	}

	constexpr void synth(float length, Frequency freq)
	{
		frame += ms2samp(length);
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
	constexpr float ms2samp(float ms)
	{
		return sample_rate * ms / 1000;
	}
};