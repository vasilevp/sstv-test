#pragma once

#include <cstdint>
#include <string>
#include <algorithm>

#include "synthesizer.hpp"

class Encoder
{
public:
	~Encoder();

	virtual void Encode() = 0;

protected:
	const uint8_t visCode;

	uint8_t *pixels;
	uint32_t width, height;
	uint32_t targetHeight;
	Synthesizer s;

	Encoder(const std::string &input, const Synthesizer &s, uint8_t visCode);

	void writeHeader();

	inline static constexpr float getY(uint8_t *pixels, size_t offset)
	{
		auto r = pixels[offset];
		auto g = pixels[offset + 1];
		auto b = pixels[offset + 2];
		return std::min(16.0 + (.003906 * ((65.738 * r) + (129.057 * g) + (25.064 * b))), 255.0);
	}

	inline static constexpr float getChromaRed(uint8_t *pixels, size_t offset)
	{
		auto r = pixels[offset];
		auto g = pixels[offset + 1];
		auto b = pixels[offset + 2];
		return std::min(128.0 + (.003906 * ((112.439 * r) + (-94.154 * g) + (-18.285 * b))), 255.0);
	}

	inline static constexpr float GetChromaBlue(uint8_t *pixels, size_t offset)
	{
		auto r = pixels[offset];
		auto g = pixels[offset + 1];
		auto b = pixels[offset + 2];
		return std::min(128.0 + (.003906 * ((-37.945 * r) + (-74.494 * g) + (112.439 * b))), 255.0);
	}
};