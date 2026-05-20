#pragma once
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

extern "C"
{
#include <C-Wav-Lib/wav.h>
}

// Reads a RIFF/WAV file into normalised mono samples in the range [-1, 1).
//
// Supports 8-bit (unsigned) and 16-bit (signed) PCM, mono or multi-channel
// (extra channels are averaged down to mono). C-Wav-Lib's own
// read_wav_samples() is not used: it has out-of-bounds indexing bugs, so the
// PCM payload is read directly here instead.
class WAVReader
{
	std::vector<float> samples_;
	uint32_t sample_rate_ = 0;

public:
	explicit WAVReader(const std::string &name)
	{
		FILE *f = name != "-" ? fopen(name.c_str(), "rb") : stdin;
		if (!f)
			throw std::runtime_error("Failed to open WAV for reading: " + name);

		wav_header_t hdr{};
		if (int err = read_wav_header(f, &hdr); err != 0)
		{
			if (f != stdin)
				fclose(f);
			throw std::runtime_error(
				"Not a valid WAV file (read_wav_header error " + std::to_string(err) + ")");
		}

		sample_rate_ = hdr.Fmt.SampleRate;
		const unsigned channels = hdr.Fmt.NumChannels;
		// Despite its name, this header field holds *bits* per sample.
		const unsigned bits = hdr.Fmt.BytesPerSample;

		if (bits != 8 && bits != 16)
		{
			if (f != stdin)
				fclose(f);
			throw std::runtime_error(
				"Unsupported sample width: " + std::to_string(bits) + "-bit (need 8 or 16)");
		}
		if (channels == 0)
		{
			if (f != stdin)
				fclose(f);
			throw std::runtime_error("WAV reports zero channels");
		}

		// read_wav_header() leaves the cursor at the start of the PCM payload
		// (byte 44). Read the rest of the file regardless of the declared
		// chunk size, which this project's encoder writes slightly short.
		std::vector<uint8_t> raw;
		uint8_t buf[4096];
		size_t n;
		while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
			raw.insert(raw.end(), buf, buf + n);

		if (f != stdin)
			fclose(f);

		const size_t bytesPerSample = bits / 8;
		const size_t frame = bytesPerSample * channels;
		const size_t frames = raw.size() / frame;

		samples_.reserve(frames);
		for (size_t fr = 0; fr < frames; ++fr)
		{
			float acc = 0.0f;
			for (unsigned c = 0; c < channels; ++c)
			{
				const uint8_t *p = raw.data() + fr * frame + c * bytesPerSample;
				if (bits == 8)
					acc += (int(p[0]) - 128) / 128.0f; // 8-bit PCM is unsigned
				else
				{
					int16_t s16 = int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
					acc += s16 / 32768.0f;
				}
			}
			samples_.push_back(acc / float(channels));
		}
	}

	const std::vector<float> &samples() const { return samples_; }
	uint32_t sampleRate() const { return sample_rate_; }
};
