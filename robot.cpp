#include "robot.hpp"

#include <cstddef>
#include <stdexcept>

#include "synthesizer.hpp"
#include "utils.hpp"

void Robot::Encode()
{
	if (height != 240)
	{
		throw std::runtime_error("Image height must be 240 pixels!");
	}

	writeHeader();
	const float pixelTime = lineTime / float(width);

	if (!greeting.empty())
		writeGreeting();

	for (size_t i = 0; i < height; ++i)
	{
		// sync pulse
		s.Synth(syncPulse, Frequency::SyncPulse);
		// sync porch
		s.Synth(syncPorch, Frequency::SyncPorch);

		for (size_t j = 0; j < width; ++j)
		{
			size_t offset = (i * width + j) * 3;
			float Y = getY(pixels, offset);
			auto freq = Synthesizer::Lerp(Y / 255);
			// pixel
			s.Synth(pixelTime, freq);
		}

		// if fullColor is set, this loop will send both colors at once
		// otherwise, for even scanlines send Cr (R-Y) and for odd scanlines send Cb (B-Y)
		for (size_t r = 0; r <= fullColor; r++)
		{
			// choose red or blue based on scanline
			bool isRedBurst = i % 2 == 0;
			// average colors between two scanlines
			auto shift = isRedBurst ? 1 : -1;

			// if both bursts need to be sent at the same time,...
			if (fullColor)
			{
				// first pass is red, second is blue
				isRedBurst = r == 0;
				// don't average colors
				shift = 0;
			}

			// red/blue sync pulse
			s.Synth(syncPulse / 2, isRedBurst ? Black : White);
			// porch
			s.Synth(syncPorch / 2, Grey);

			auto getChroma = (isRedBurst ? getChromaRed : GetChromaBlue);
			for (size_t j = 0; j < width; ++j)
			{
				// get current pixel chroma, averaging between two lines if needed
				float c1 = getChroma(pixels, (i * width + j) * 3);
				float c2 = getChroma(pixels, ((i + shift) * width + j) * 3);
				auto freq = Synthesizer::Lerp((c1 + c2) / 2 / 255);

				// send pixel
				s.Synth(pixelTime / 2, freq);
			}
		}
	}
}

void Robot::writeGreeting()
{
	const float pixelTime = lineTime / float(width);

	for (size_t i = 0; i < 16; ++i)
	{
		// sync pulse
		s.Synth(syncPulse, SyncPulse);
		// sync porch
		s.Synth(syncPorch, SyncPorch);
		// line
		for (size_t j = 0; j < width; ++j)
		{
			auto set = utils::getText(i, j, 2, greeting);
			s.Synth(pixelTime, set ? White : Black);
		}

		for (size_t k = 0; k <= fullColor; k++)
		{
			bool even = i % 2 == 0;
			if (fullColor)
			{
				even = k == 0;
			}

			s.Synth(syncPulse / 2, even ? Black : White);
			// sync porch
			s.Synth(syncPorch / 2, Grey);
			// line
			s.Synth(pixelTime / 2 * float(width), Grey);
		}
	}
}