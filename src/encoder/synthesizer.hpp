#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "sample_sink.hpp"
#include "utils.hpp"

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

	// The sink is borrowed, not owned — the caller is responsible for its
	// lifetime (and for calling sink->finish() after the encode completes;
	// the Encoder base does that on Synthesizer's behalf).
	Synthesizer(SampleSink &sink, std::uint32_t sample_rate = 8000)
		: sink(&sink),
		  sample_rate(sample_rate),
		  freq_step(sample_rate / utils::lut.size())
	{
		utils::Guard();
	}

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
			const std::uint8_t x = utils::lut[(idx / freq_step) % utils::lut.size()] + 128;
			idx += freq;
			sink->put(x);
		}
	}

	std::uint32_t sampleRate() const { return sample_rate; }

private:
	SampleSink *sink;
	std::uint32_t sample_rate;
	std::uint32_t freq_step;

	float frame = 0;
	std::uint32_t idx = 0;

	inline constexpr float ms2samp(float ms)
	{
		return sample_rate * ms / 1000;
	}
};
