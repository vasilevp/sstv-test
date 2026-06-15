#pragma once
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include "sample_sink.hpp"

// Streaming WAV writer for 8-bit unsigned PCM mono audio. The previous
// WAVWriter buffered every sample in a std::vector and dumped them all in
// its destructor; this sink writes a placeholder RIFF/data header at
// construction, streams every put() byte straight to disk through fwrite,
// and patches the RIFF chunk size and the data chunk size at finish() via
// fseek. Memory in flight is one byte at a time (or one block, for the
// span overload).
//
// 0.5 s of silence is written before and after the SSTV signal — same
// padding the previous WAVWriter applied so off-the-shelf players don't
// click on first/last frame.
class WAVFileSink : public SampleSink
{
public:
	WAVFileSink(const std::string &path, std::uint32_t sampleRate);
	~WAVFileSink() override;

	WAVFileSink(const WAVFileSink &) = delete;
	WAVFileSink &operator=(const WAVFileSink &) = delete;

	void put(std::uint8_t sample) override;
	void put(std::span<const std::uint8_t> samples) override;
	void finish() override;

private:
	void writeBytes(const void *data, std::size_t size);
	void writeSilence(std::uint32_t sampleCount);

	std::string path;
	std::FILE *file = nullptr;
	std::uint32_t sampleRate;
	std::uint32_t bytesWritten = 0; // signal samples + the two silence pads
	bool finished = false;
};
