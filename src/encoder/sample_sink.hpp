#pragma once
#include <cstdint>
#include <span>

// Push interface for 8-bit unsigned PCM samples. The Synthesizer emits one
// sample at a time; sinks may buffer internally or write straight through.
// finish() is called once when the encode completes so seekable sinks (the
// WAV file, which patches its data-size header field at the end) can
// finalise.
class SampleSink
{
public:
	virtual ~SampleSink() = default;

	// One unsigned-PCM sample byte (centre = 128).
	virtual void put(std::uint8_t sample) = 0;

	// A block of unsigned-PCM sample bytes. Default implementation just
	// loops on put() — overrides should use it to write whole blocks at a
	// time when the underlying device benefits (e.g. fwrite on stdio).
	virtual void put(std::span<const std::uint8_t> samples)
	{
		for (std::uint8_t s : samples)
			put(s);
	}

	// No more samples will be written. Implementations should flush any
	// pending state (close files, patch headers, drain DMA, ...).
	virtual void finish() = 0;
};
