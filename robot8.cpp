#include "robot8.hpp"

#include <cstddef>
#include <stdexcept>

#include "synthesizer.hpp"
#include "utils.hpp"

void Robot8::Encode()
{
	if ((height % 120) != 0)
	{
		throw std::runtime_error("Image height must be divisible by 120!");
	}

	writeHeader();

	if (!greeting.empty())
		writeGreeting();

	const float pixelTime = lineTime / float(width);
	const int stride = int(float(height) / 120);

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

void Robot8::writeGreeting()
{
	const float pixelTime = lineTime / float(width);

	auto textLine = [&](auto i)
	{
		// sync pulse
		s.synth(syncTime, SyncPulse);

		for (size_t j = 0; j < width; ++j)
		{
			auto set = utils::getText(i, j, 1, greeting);
			s.synth(pixelTime, set ? White : Black);
		}
	};

	for (size_t i = 0; i < 8; ++i)
	{
		textLine(i);
	}
}
