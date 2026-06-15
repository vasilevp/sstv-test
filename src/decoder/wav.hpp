#pragma once
#include <cstdint>
#include <cstdio>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "sample_source.hpp"

extern "C"
{
#include <C-Wav-Lib/wav.h>
}

// Streaming RIFF/WAV reader. The header is parsed once at construction;
// subsequent read() calls pull PCM bytes from the file in chunks and convert
// them to normalised mono floats on the fly, so the whole recording never
// has to live in memory.
//
// Supports 8-bit (unsigned) and 16-bit (signed) PCM, mono or multi-channel
// (extra channels are averaged down to mono). C-Wav-Lib's own
// read_wav_samples() is not used: it has out-of-bounds indexing bugs, so
// the PCM payload is read directly here instead.
class WAVReader : public SampleSource
{
public:
	explicit WAVReader(const std::string &name)
	{
		file = name != "-" ? std::fopen(name.c_str(), "rb") : stdin;
		if (!file)
			throw std::runtime_error("Failed to open WAV for reading: " + name);

		wav_header_t hdr{};
		if (int err = read_wav_header(file, &hdr); err != 0)
		{
			closeFile();
			throw std::runtime_error(
				"Not a valid WAV file (read_wav_header error " + std::to_string(err) + ")");
		}

		rate = hdr.Fmt.SampleRate;
		channels = hdr.Fmt.NumChannels;
		// Despite its name, this header field holds *bits* per sample.
		bits = hdr.Fmt.BytesPerSample;

		if (bits != 8 && bits != 16)
		{
			closeFile();
			throw std::runtime_error(
				"Unsupported sample width: " + std::to_string(bits) + "-bit (need 8 or 16)");
		}
		if (channels == 0)
		{
			closeFile();
			throw std::runtime_error("WAV reports zero channels");
		}
	}

	~WAVReader() override { closeFile(); }

	WAVReader(const WAVReader &) = delete;
	WAVReader &operator=(const WAVReader &) = delete;

	std::size_t read(std::span<float> out) override
	{
		if (!file)
			return 0;

		const std::size_t bytesPerSample = bits / 8;
		const std::size_t frameBytes = bytesPerSample * channels;
		const std::size_t maxFrames = out.size();
		if (maxFrames == 0)
			return 0;

		const std::size_t wantBytes = maxFrames * frameBytes;
		if (raw.size() < wantBytes)
			raw.resize(wantBytes);

		const std::size_t got = std::fread(raw.data(), 1, wantBytes, file);
		const std::size_t frames = got / frameBytes;

		for (std::size_t fr = 0; fr < frames; ++fr)
		{
			float acc = 0.0f;
			for (unsigned c = 0; c < channels; ++c)
			{
				const std::uint8_t *p = raw.data() + fr * frameBytes + c * bytesPerSample;
				if (bits == 8)
					acc += (int(p[0]) - 128) / 128.0f; // 8-bit PCM is unsigned
				else
				{
					std::int16_t s16 = std::int16_t(std::uint16_t(p[0]) |
													(std::uint16_t(p[1]) << 8));
					acc += s16 / 32768.0f;
				}
			}
			out[fr] = acc / float(channels);
		}
		return frames;
	}

	std::uint32_t sampleRate() const override { return rate; }

private:
	void closeFile()
	{
		if (file && file != stdin)
			std::fclose(file);
		file = nullptr;
	}

	std::FILE *file = nullptr;
	std::uint32_t rate = 0;
	unsigned channels = 0;
	unsigned bits = 0;
	std::vector<std::uint8_t> raw;
};
