#include "robot8.hpp"

#include <stdexcept>
#include <cstdint>

#include "utils.hpp"

#include "LoadBMP/loadbmp.h"

void Robot8::Encode()
{
	if ((height % 120) != 0)
	{
		throw std::runtime_error("Image height must be divisible by 120!");
	}

	writeHeader();

	writeGreyscale();

	const float pixelTime = lineTime / width;
	const int stride = height / 120;

	for (size_t i = 0; i < height; i += stride)
	{
		// sync pulse
		s.synth(syncTime, SyncPulse);

		for (size_t j = 0; j < width; ++j)
		{
			size_t offset = (i * width + j) * 3;
			float Y = getY(pixels, offset);

			// pixel
			s.synth(pixelTime, s.Lerp(Y / 255));
		}
	}
}

void Robot8::writeGreyscale()
{
	const float syncTime = 5;
	const float pixelTime = 56.0f / width;

	for (size_t i = 0; i < 8; ++i)
	{
		// sync pulse
		s.synth(syncTime, SyncPulse);

		for (size_t j = 0; j < width; ++j)
		{
			size_t offset = (i * width + j) * 3;
			float val = float(j) / width;

			auto freq = Synthesizer::Lerp(val);

			// pixel
			s.synth(pixelTime, freq);
		}
	}
}