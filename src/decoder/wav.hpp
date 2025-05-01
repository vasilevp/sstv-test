#pragma once
#include <cstdint>
#include <string>
#include <vector>

extern "C"
{
#include <C-Wav-Lib/wav.h>
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
