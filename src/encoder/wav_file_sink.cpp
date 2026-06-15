#include "wav_file_sink.hpp"

#include <cstring>
#include <stdexcept>

namespace
{
	void writeU16LE(unsigned char *p, std::uint16_t v)
	{
		p[0] = std::uint8_t(v);
		p[1] = std::uint8_t(v >> 8);
	}

	void writeU32LE(unsigned char *p, std::uint32_t v)
	{
		p[0] = std::uint8_t(v);
		p[1] = std::uint8_t(v >> 8);
		p[2] = std::uint8_t(v >> 16);
		p[3] = std::uint8_t(v >> 24);
	}
}

WAVFileSink::WAVFileSink(const std::string &path, std::uint32_t sampleRate)
	: path(path), sampleRate(sampleRate)
{
	file = path != "-" ? std::fopen(path.c_str(), "wb") : stdout;
	if (!file)
		throw std::runtime_error("Failed to open WAV for writing: " + path);

	// 44-byte canonical PCM WAV header (RIFF + WAVE + fmt + data). The
	// RIFF chunk size (offset 4) and the data chunk size (offset 40) stay
	// as zero placeholders until finish() patches them.
	unsigned char hdr[44] = {};
	std::memcpy(hdr + 0,  "RIFF", 4);
	writeU32LE(hdr + 4, 0);
	std::memcpy(hdr + 8,  "WAVE", 4);
	std::memcpy(hdr + 12, "fmt ", 4);
	writeU32LE(hdr + 16, 16);          // fmt chunk body size
	writeU16LE(hdr + 20, 1);           // format = PCM
	writeU16LE(hdr + 22, 1);           // channels = mono
	writeU32LE(hdr + 24, sampleRate);
	writeU32LE(hdr + 28, sampleRate);  // byte rate = rate * channels * bytes/sample
	writeU16LE(hdr + 32, 1);           // block align
	writeU16LE(hdr + 34, 8);           // bits per sample
	std::memcpy(hdr + 36, "data", 4);
	writeU32LE(hdr + 40, 0);

	// Header bytes don't count toward the data-chunk size; write them
	// directly so `bytesWritten` only tracks audio payload.
	if (std::fwrite(hdr, 1, sizeof(hdr), file) != sizeof(hdr))
		throw std::runtime_error("WAVFileSink header write failed: " + path);

	// Pad the start with 0.5 s of silence so off-the-shelf players don't
	// click on first frame — matches the previous WAVWriter's behaviour.
	writeSilence(sampleRate / 2);
}

WAVFileSink::~WAVFileSink()
{
	// Best-effort close — if the caller forgot to call finish() we still
	// release the file (the header will be wrong but the OS won't leak it).
	if (file && file != stdout)
		std::fclose(file);
}

void WAVFileSink::put(std::uint8_t sample)
{
	if (finished)
		throw std::runtime_error("WAVFileSink::put after finish");
	writeBytes(&sample, 1);
}

void WAVFileSink::put(std::span<const std::uint8_t> samples)
{
	if (finished)
		throw std::runtime_error("WAVFileSink::put after finish");
	writeBytes(samples.data(), samples.size());
}

void WAVFileSink::finish()
{
	if (!file || finished)
		return;
	finished = true;

	// Trailing 0.5 s of silence to match the prior writer.
	writeSilence(sampleRate / 2);

	// Patch the two size fields in the header. RIFF chunk size is the
	// total file length minus 8 (the "RIFF" magic + this size field).
	unsigned char sizeLE[4];
	writeU32LE(sizeLE, bytesWritten);
	std::fseek(file, 40, SEEK_SET);
	std::fwrite(sizeLE, 1, 4, file);

	writeU32LE(sizeLE, bytesWritten + 36);
	std::fseek(file, 4, SEEK_SET);
	std::fwrite(sizeLE, 1, 4, file);

	if (file != stdout)
		std::fclose(file);
	file = nullptr;
}

void WAVFileSink::writeBytes(const void *data, std::size_t size)
{
	if (size == 0)
		return;
	if (std::fwrite(data, 1, size, file) != size)
		throw std::runtime_error("WAVFileSink short write: " + path);
	// Header bytes (the first 44) don't count toward the data-chunk size.
	if (bytesWritten + size < bytesWritten) // (paranoia: catch overflow)
		throw std::runtime_error("WAVFileSink size overflow: " + path);
	bytesWritten += static_cast<std::uint32_t>(size);
}

void WAVFileSink::writeSilence(std::uint32_t sampleCount)
{
	constexpr std::uint8_t centre = 128;
	constexpr std::size_t chunk = 256;
	std::uint8_t buf[chunk];
	std::memset(buf, centre, chunk);
	while (sampleCount > 0)
	{
		const std::size_t n = std::min<std::size_t>(sampleCount, chunk);
		writeBytes(buf, n);
		sampleCount -= std::uint32_t(n);
	}
}
