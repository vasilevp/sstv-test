#pragma once
#include <cstdint>
#include <span>

// Pull interface for audio samples. The decoder doesn't read from the input
// directly; it consumes whatever a SampleSource hands back, one block at a
// time. WAVReader implements it for a file-on-disk; an embedded port would
// implement it on top of I2S/ADC and feed audio in chunks as DMA hands them
// back.
class SampleSource
{
public:
	virtual ~SampleSource() = default;

	// Fill `out` with up to out.size() normalised mono samples ([-1, 1)).
	// Returns the count actually read; 0 means end of stream. Implementations
	// may return less than out.size() before EOF (e.g. only as much as one
	// ADC DMA chunk holds), so callers must loop until they get 0.
	virtual std::size_t read(std::span<float> out) = 0;

	// Sample rate of the stream, in Hz. Constant for the lifetime of the
	// source.
	virtual std::uint32_t sampleRate() const = 0;
};
