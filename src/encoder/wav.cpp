#include "wav.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utils.hpp"

extern "C"
{
#include <C-Wav-Lib/wav.h>
};

WAVWriter::WAVWriter(WAVWriter &&other) noexcept : sample_rate(other.sample_rate), f(other.f), samples(std::move(other.samples))
{
	utils::Guard();
	other.f = nullptr; // Transfer ownership
}

WAVWriter::~WAVWriter()
{
	utils::Guard();

	if (!f)
		return;

	write_wav_header(
		f,
		1,
		sample_rate,
		1,
		samples.size() + sample_rate / 10 * 2);

	std::vector<uint8_t> silence(sample_rate / 2, 128);

	// surround actual data with silence for easier playback
	wav_sample_t sample{
		.BytesPerSample = 1,
		.Channels = 1,
	};

	sample.DataSize = silence.size();
	sample.sampleData = silence.data();
	write_wav_sample(f, &sample);

	sample.DataSize = samples.size();
	sample.sampleData = samples.data();
	write_wav_sample(f, &sample);

	sample.DataSize = silence.size();
	sample.sampleData = silence.data();
	write_wav_sample(f, &sample);

	fclose(f);
}

WAVWriter::WAVWriter(const std::string &name, const size_t sample_rate)
	: sample_rate(sample_rate)
{
	utils::Guard();

	f = name != "-" ? fopen(name.c_str(), "wb") : stdout;
	if (!f)
		throw std::runtime_error("Failed to open file for writing");

	samples.reserve(sample_rate);
}

void WAVWriter::put(uint8_t sample)
{
	// don't care about RAM, care about speed
	if (samples.capacity() == samples.size())
		samples.reserve(samples.capacity() * 2);

	samples.push_back(sample);
}
