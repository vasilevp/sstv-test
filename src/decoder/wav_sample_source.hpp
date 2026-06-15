#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "sample_source.hpp"

// Streaming RIFF/WAV reader. The header is parsed once at construction;
// subsequent read() calls pull sample bytes from the file in chunks and
// convert them to normalised mono floats on the fly, so the whole recording
// never has to live in memory.
//
// The chunk list after "WAVE" is walked in order, so the fmt and data chunks
// need not sit at fixed offsets — interspersed fact/PEAK/LIST/INFO chunks are
// skipped. Both integer PCM (8/16/24/32-bit) and IEEE float (32/64-bit) sample
// formats are supported, mono or multi-channel; extra channels are averaged
// down to mono on read(). Reads are bounded by the data chunk's declared size,
// so trailing chunks after the audio are never mistaken for samples.
class WAVSampleSource : public SampleSource
{
public:
	explicit WAVSampleSource(const std::string &name)
	{
		file = name != "-" ? std::fopen(name.c_str(), "rb") : stdin;
		if (!file)
			throw std::runtime_error("Failed to open WAV for reading: " + name);

		auto u16 = [](const unsigned char *p)
		{ return std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8); };
		auto u32 = [](const unsigned char *p)
		{
			return std::uint32_t(p[0]) |
			       (std::uint32_t(p[1]) << 8) |
			       (std::uint32_t(p[2]) << 16) |
			       (std::uint32_t(p[3]) << 24);
		};

		// RIFF/WAVE container header.
		unsigned char riff[12];
		if (std::fread(riff, 1, sizeof(riff), file) != sizeof(riff))
		{
			closeFile();
			throw std::runtime_error("Truncated WAV header: " + name);
		}
		if (std::memcmp(riff + 0, "RIFF", 4) != 0 ||
		    std::memcmp(riff + 8, "WAVE", 4) != 0)
		{
			closeFile();
			throw std::runtime_error("Not a RIFF/WAVE file: " + name);
		}

		// Walk the chunk list until the data chunk, parsing fmt along the way.
		std::uint16_t format = 0;
		bool haveFmt = false;
		for (;;)
		{
			unsigned char head[8];
			if (std::fread(head, 1, sizeof(head), file) != sizeof(head))
			{
				closeFile();
				throw std::runtime_error("WAV has no data chunk: " + name);
			}
			const std::uint32_t chunkSize = u32(head + 4);

			if (std::memcmp(head, "fmt ", 4) == 0)
			{
				unsigned char fmt[40] = {};
				const std::uint32_t want = std::min<std::uint32_t>(chunkSize, sizeof(fmt));
				if (std::fread(fmt, 1, want, file) != want)
				{
					closeFile();
					throw std::runtime_error("Truncated fmt chunk: " + name);
				}
				format = u16(fmt + 0);
				channels = u16(fmt + 2);
				rate = u32(fmt + 4);
				bits = u16(fmt + 14);
				// WAVE_FORMAT_EXTENSIBLE stows the real format tag in the first
				// two bytes of the subformat GUID.
				if (format == 0xFFFE && want >= 26)
					format = u16(fmt + 24);
				haveFmt = true;
				skip(chunkSize - want + (chunkSize & 1));
			}
			else if (std::memcmp(head, "data", 4) == 0)
			{
				if (!haveFmt)
				{
					closeFile();
					throw std::runtime_error("WAV data chunk precedes fmt: " + name);
				}
				dataRemaining = chunkSize;
				break; // file is now positioned at the first sample
			}
			else
			{
				skip(chunkSize + (chunkSize & 1)); // chunks are word-aligned
			}
		}

		isFloat = format == 3;
		const bool okFloat = isFloat && (bits == 32 || bits == 64);
		const bool okPcm = format == 1 && (bits == 8 || bits == 16 || bits == 24 || bits == 32);
		if (!okFloat && !okPcm)
		{
			closeFile();
			throw std::runtime_error(
				"Unsupported WAV sample format (tag " + std::to_string(format) +
				", " + std::to_string(bits) + "-bit): " + name);
		}
		if (channels == 0)
		{
			closeFile();
			throw std::runtime_error("WAV reports zero channels: " + name);
		}
	}

	~WAVSampleSource() override { closeFile(); }

	WAVSampleSource(const WAVSampleSource &) = delete;
	WAVSampleSource &operator=(const WAVSampleSource &) = delete;

	std::size_t read(std::span<float> out) override
	{
		if (!file)
			return 0;

		const std::size_t bytesPerSample = bits / 8;
		const std::size_t frameBytes = bytesPerSample * channels;
		const std::size_t maxFrames = out.size();
		if (maxFrames == 0)
			return 0;

		std::size_t wantBytes = maxFrames * frameBytes;
		if (dataRemaining < wantBytes)
			wantBytes = std::size_t(dataRemaining);
		if (raw.size() < wantBytes)
			raw.resize(wantBytes);

		const std::size_t got = std::fread(raw.data(), 1, wantBytes, file);
		dataRemaining -= got;
		const std::size_t frames = got / frameBytes;

		for (std::size_t fr = 0; fr < frames; ++fr)
		{
			float acc = 0.0f;
			for (unsigned c = 0; c < channels; ++c)
				acc += sample(raw.data() + fr * frameBytes + c * bytesPerSample);
			out[fr] = acc / float(channels);
		}
		return frames;
	}

	std::uint32_t sampleRate() const override { return rate; }

private:
	// Decode one channel sample at `p` to a normalised float in [-1, 1].
	float sample(const std::uint8_t *p) const
	{
		if (isFloat)
		{
			if (bits == 32)
			{
				float f;
				std::memcpy(&f, p, sizeof(f));
				return f;
			}
			double d;
			std::memcpy(&d, p, sizeof(d));
			return float(d);
		}
		switch (bits)
		{
		case 8: // 8-bit PCM is unsigned, centred on 128
			return (int(p[0]) - 128) / 128.0f;
		case 16:
		{
			std::int16_t s = std::int16_t(std::uint16_t(p[0]) |
			                              (std::uint16_t(p[1]) << 8));
			return s / 32768.0f;
		}
		case 24:
		{
			std::int32_t s = std::int32_t((std::uint32_t(p[0]) << 8) |
			                              (std::uint32_t(p[1]) << 16) |
			                              (std::uint32_t(p[2]) << 24));
			return (s >> 8) / 8388608.0f; // arithmetic shift sign-extends
		}
		default: // 32-bit PCM
		{
			std::int32_t s = std::int32_t(std::uint32_t(p[0]) |
			                              (std::uint32_t(p[1]) << 8) |
			                              (std::uint32_t(p[2]) << 16) |
			                              (std::uint32_t(p[3]) << 24));
			return s / 2147483648.0f;
		}
		}
	}

	// Advance the read position past `n` bytes, working on pipes too.
	void skip(std::uint32_t n)
	{
		unsigned char scratch[512];
		while (n > 0)
		{
			const std::size_t want = std::min<std::size_t>(n, sizeof(scratch));
			const std::size_t got = std::fread(scratch, 1, want, file);
			if (got == 0)
				break; // EOF mid-skip; the next read() will report end of stream
			n -= std::uint32_t(got);
		}
	}

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
	bool isFloat = false;
	std::uint64_t dataRemaining = 0;
	std::vector<std::uint8_t> raw;
};
