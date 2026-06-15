#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>

#include "pixel_source.hpp"
#include "synthesizer.hpp"

class Encoder
{
public:
	virtual ~Encoder() = default;

	virtual void Encode() = 0;

protected:
	const uint8_t visCode;

	// Pixel input and audio output are both injected — the pixel rows are
	// pulled on demand from `pixels` (the streaming BMP source caches the
	// two most-recently read rows for the encoders that need adjacent-row
	// access), and emitted samples flow through the Synthesizer into the
	// sink it was constructed over.
	PixelSource &pixels;
	uint32_t width;
	uint32_t height;
	uint32_t targetHeight;
	Synthesizer s;

	Encoder(PixelSource &source, Synthesizer &&s, uint8_t visCode)
		: visCode(visCode),
		  pixels(source),
		  width(source.width()),
		  height(source.height()),
		  s(std::move(s))
	{
	}

	void writeHeader();

	// All three helpers take a 3-byte span (R, G, B) — one pixel's worth.
	// Mode encoders compute the pixel's column index and pass
	// `row.subspan(x * 3, 3)`.
	inline static constexpr float getY(std::span<const std::uint8_t> px)
	{
		const auto r = px[0];
		const auto g = px[1];
		const auto b = px[2];
		return std::min(16.0 + (.003906 * ((65.738 * r) + (129.057 * g) + (25.064 * b))), 255.0);
	}

	inline static constexpr float getChromaRed(std::span<const std::uint8_t> px)
	{
		const auto r = px[0];
		const auto g = px[1];
		const auto b = px[2];
		return std::min(128.0 + (.003906 * ((112.439 * r) + (-94.154 * g) + (-18.285 * b))), 255.0);
	}

	inline static constexpr float GetChromaBlue(std::span<const std::uint8_t> px)
	{
		const auto r = px[0];
		const auto g = px[1];
		const auto b = px[2];
		return std::min(128.0 + (.003906 * ((-37.945 * r) + (-74.494 * g) + (112.439 * b))), 255.0);
	}
};
