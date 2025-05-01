#pragma once
#include <cmath>
#include <stdint.h>

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
	Synthesizer(const Synthesizer &) = delete;
	Synthesizer &operator=(const Synthesizer &) = delete;

	Synthesizer(Synthesizer &&) = default;
	Synthesizer &operator=(Synthesizer &&) = default;

	Synthesizer(const std::string &output, size_t sample_rate = 8000)
		: sample_rate(sample_rate),
		  freq_step(sample_rate / utils::lut.size()),
		  w{output, sample_rate}
	{
		utils::Guard();
	};

	Synthesizer(const char output[], size_t sample_rate = 8000)
		: sample_rate(sample_rate),
		  freq_step(sample_rate / utils::lut.size()),
		  w{output, sample_rate}
	{
		utils::Guard();
	};

	inline static constexpr Frequency Lerp(Frequency from, Frequency to, float f)
	{
		return Frequency(from + Frequency(float(to - from) * f));
	}

	inline static constexpr Frequency Lerp(float f)
	{
		return Lerp(Black, White, f);
	}

	inline constexpr void Synth(float length, Frequency freq)
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
	size_t sample_rate;
	int freq_step;

	float frame = 0;
	int idx = 0;
	inline constexpr float ms2samp(float ms)
	{
		return sample_rate * ms / 1000;
	}
};