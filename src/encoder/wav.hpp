#pragma once
#include <iostream>
#include <stacktrace>
#include <string>
#include <vector>

extern "C"
{
#include <C-Wav-Lib/wav.h>
};

class WAVWriter
{
	size_t sample_rate;

	FILE *f = nullptr;

	std::vector<uint8_t> samples;
	wav_sample_t sample = {1, 1, 1, nullptr};

public:
	// Avoid copying because we own *f
	WAVWriter(const WAVWriter &) = delete;
	WAVWriter &operator=(const WAVWriter &) = delete;

	// Move constructor
	WAVWriter(WAVWriter &&other) noexcept
		: sample_rate(other.sample_rate), f(other.f), samples(std::move(other.samples)), sample(other.sample)
	{
		utils::Guard();
		other.f = nullptr; // Transfer ownership
	}

	WAVWriter &operator=(WAVWriter &&other) = default;

	WAVWriter(const std::string &name, const size_t sample_rate)
		: sample_rate(sample_rate)
	{
		utils::Guard();

		f = name != "-" ? fopen(name.c_str(), "wb") : stdout;
		if (!f)
			throw std::runtime_error("Failed to open file for writing");
		samples.reserve(sample_rate);
	}

	~WAVWriter()
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

	void put(uint8_t sample)
	{
		// don't care about RAM, care about speed
		if (samples.capacity() == samples.size())
			samples.reserve(samples.capacity() * 2);

		samples.push_back(sample);
	}

	size_t size() const
	{
		return samples.size();
	}
};
class WAVReader
{
	std::vector<uint32_t> samples;
	wav_sample_t sample = {1, 1, 1, nullptr};

public:
	WAVReader(const std::string &name)
	{
		FILE *f = name != "-" ? fopen(name.c_str(), "rb") : stdin;
		wav_header_t hdr;
		read_wav_header(f, &hdr);
		std::vector<uint32_t> samples_tmp;
		samples_tmp.reserve(hdr.Data.Subchunk2Size);
		read_wav_samples(f, samples.data(), hdr.Data.Subchunk2Size, 1, 0);
	}
};