#include "martin.hpp"

#include <stdexcept>
#include <cstdint>
#include <iostream>
#include <print>

#include "utils.hpp"

#include "modules/LoadBMP/loadbmp.h"

using namespace std;

void Martin::Encode()
{
	if (height != 256)
	{
		print(cerr, "WARN: Image height must be 256 pixels! Provided height of {} may cause issues with the receiver.", height);
	}

	writeHeader();
	if (!greeting.empty())
		writeGreeting();

	const float pixelTime = lineTime * (3 - mode) / width;

	for (size_t i = 0; i < height; ++i)
	{
		// sync pulse
		s.synth(syncPulse, SyncPulse);

		colorLine(i, 1);
		colorLine(i, 2);
		colorLine(i, 0);

		// separator
		s.synth(syncPorch, SyncPorch);
	}
}

void Martin::colorLine(int i, size_t color)
{
	const float pixelTime = lineTime * (3 - mode) / width;

	// sync porch
	s.synth(syncPorch, Frequency::SyncPorch);

	for (size_t j = 0; j < width; ++j)
	{
		size_t offset = (i * width + j) * 3;
		float c = pixels[offset + color];
		auto freq = Synthesizer::Lerp(c / 255);

		// pixel
		s.synth(pixelTime, freq);
	}
};

void Martin::writeGreeting()
{
	const float pixelTime = lineTime * (3 - mode) / width;

	auto textline = [&](int i)
	{
		// sync porch
		s.synth(syncPorch, Frequency::SyncPorch);

		for (size_t j = 0; j < width; ++j)
		{
			auto set = utils::getText(i, j, 2, greeting);
			s.synth(pixelTime, set ? White : Black);
		}
	};

	for (size_t i = 0; i < 16; ++i)
	{
		// sync pulse
		s.synth(syncPulse, SyncPulse);

		textline(i);
		textline(i);
		textline(i);

		// separator
		s.synth(syncPorch, SyncPorch);
	}
}