#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "sample_source.hpp"

// Streaming RIFF/WAV reader. The header is parsed once at construction;
// subsequent read() calls pull PCM bytes from the file in chunks and convert
// them to normalised mono floats on the fly, so the whole recording never
// has to live in memory.
//
// Supports the canonical 44-byte PCM/RIFF header layout (RIFF + WAVE + fmt
// + data, in that order) for 8-bit (unsigned) and 16-bit (signed) samples,
// mono or multi-channel. Extra channels are averaged down to mono on
// read(). Files with extra chunks (LIST/INFO, fact, ...) or non-16-byte
// fmt chunks are rejected — the project's encoder doesn't write those.
class WAVReader : public SampleSource
{
public:
	explicit WAVReader(const std::string &name)
	{
		file = name != "-" ? std::fopen(name.c_str(), "rb") : stdin;
		if (!file)
			throw std::runtime_error("Failed to open WAV for reading: " + name);

		// Read the canonical 44-byte RIFF/WAVE/fmt/data header and validate
		// every field that gates the streaming PCM decode below.
		unsigned char hdr[44];
		if (std::fread(hdr, 1, sizeof(hdr), file) != sizeof(hdr))
		{
			closeFile();
			throw std::runtime_error("Truncated WAV header: " + name);
		}
		auto u16 = [](const unsigned char *p)
		{ return std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8); };
		auto u32 = [](const unsigned char *p)
		{
			return std::uint32_t(p[0]) |
			       (std::uint32_t(p[1]) << 8) |
			       (std::uint32_t(p[2]) << 16) |
			       (std::uint32_t(p[3]) << 24);
		};
		if (std::memcmp(hdr + 0,  "RIFF", 4) != 0 ||
		    std::memcmp(hdr + 8,  "WAVE", 4) != 0 ||
		    std::memcmp(hdr + 12, "fmt ", 4) != 0 ||
		    std::memcmp(hdr + 36, "data", 4) != 0)
		{
			closeFile();
			throw std::runtime_error("Not a canonical RIFF/PCM WAV: " + name);
		}
		const std::uint32_t fmtSize = u32(hdr + 16);
		const std::uint16_t format = u16(hdr + 20);
		if (fmtSize != 16 || format != 1)
		{
			closeFile();
			throw std::runtime_error(
				"Unsupported WAV format (need PCM, 16-byte fmt chunk): " + name);
		}

		channels = u16(hdr + 22);
		rate = u32(hdr + 24);
		bits = u16(hdr + 34);

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
