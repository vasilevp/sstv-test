#include "robot36.hpp"

#include <stdexcept>
#include <cstdint>

#include "utils.hpp"

#include "LoadBMP/loadbmp.h"

void Robot::Encode()
{
	if (height != 240)
	{
		throw std::runtime_error("Image height must be 240 pixels!");
	}

	writeHeader();
	const float pixelTime = lineTime / width;

	writeGreyscale();

	for (size_t i = 0; i < height; ++i)
	{
		// sync pulse
		s.synth(syncPulse, Frequency::SyncPulse);
		// sync porch
		s.synth(syncPorch, Frequency::SyncPorch);

		for (size_t j = 0; j < width; ++j)
		{
			size_t offset = (i * width + j) * 3;
			float Y = getY(pixels, offset);
			auto freq = Synthesizer::Lerp(Y / 255);
			// pixel
			s.synth(pixelTime, freq);
		}

		// if repeat is set, this loop will send both colors at once
		// otherwise, for even scanlines send Cr (R-Y) and for odd scanlines send Cb (B-Y)
		for (size_t r = 0; r <= repeat; r++)
		{
			// choose red or blue based on scanline
			bool isRedBurst = i % 2 == 0;
			// average colors between two scanlines
			auto shift = isRedBurst ? 1 : -1;

			// if both bursts need to be sent at the same time,...
			if (repeat)
			{
				// first pass is red, second is blue
				isRedBurst = r == 0;
				// don't average colors
				shift = 0;
			}

			auto getChroma = (isRedBurst ? getChromaRed : GetChromaBlue);

			// red/blue sync pulse
			s.synth(syncPulse / 2, isRedBurst ? Black : White);
			// porch
			s.synth(syncPorch / 2, Grey);

			for (size_t j = 0; j < width; ++j)
			{
				// get current pixel chroma, averaging between two lines if needed
				float c1 = getChroma(pixels, (i * width + j) * 3);
				float c2 = getChroma(pixels, ((i + shift) * width + j) * 3);
				auto freq = Synthesizer::Lerp((c1 + c2) / 2 / 255);

				// send pixel
				s.synth(pixelTime / 2, freq);
			}
		}
	}
}

void Robot::writeGreyscale()
{
	const float pixelTime = lineTime / width;

	for (size_t i = 0; i < greyscaleLines; ++i)
	{
		// sync pulse
		s.synth(syncPulse, SyncPulse);
		// sync porch
		s.synth(syncPorch, SyncPorch);
		// line
		for (size_t j = 0; j < width; ++j)
		{
			float Y = float(j * 255) / width;
			auto freq = Synthesizer::Lerp(Y / 255);

			// pixel
			s.synth(pixelTime, freq);
		}

		for (size_t k = 0; k <= repeat; k++)
		{
			bool even = i % 2 == 0;
			if (repeat)
			{
				even = k == 0;
			}

			s.synth(syncPulse / 2, even ? Black : White);
			// sync porch
			s.synth(syncPorch / 2, Grey);
			// line
			s.synth(pixelTime / 2 * width, Grey);
		}
	}
}