#include "martin.hpp"
#include "synthesizer.hpp"
#include "utils.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>

using namespace std;

void Martin::Encode()
{
	utils::Guard();

	if (height != 256)
	{
		std::print(cerr, "WARN: Image height must be 256 pixels! Provided height of {} may cause issues with the receiver.", height);
	}

	writeHeader();
	if (!greeting.empty())
		writeGreeting();

	for (uint32_t i = 0; i < height; ++i)
	{
		// sync pulse
		s.Synth(syncPulse, SyncPulse);

		colorLine(i, 1);
		colorLine(i, 2);
		colorLine(i, 0);

		// separator
		s.Synth(syncPorch, SyncPorch);
	}
}

void Martin::colorLine(uint32_t i, size_t color)
{
	const float pixelTime = lineTime * float(3 - mode) / float(width);

	// sync porch
	s.Synth(syncPorch, Frequency::SyncPorch);

	for (size_t j = 0; j < width; ++j)
	{
		const size_t offset = (i * width + j) * 3;
		const float c = pixels[offset + color];
		auto freq = Synthesizer::Lerp(c / 255);

		// pixel
		s.Synth(pixelTime, freq);
	}
};

void Martin::writeGreeting()
{
	utils::Guard();

	const float pixelTime = lineTime * float(3 - mode) / float(width);

	auto textline = [&](int i)
	{
		// sync porch
		s.Synth(syncPorch, Frequency::SyncPorch);

		for (size_t j = 0; j < width; ++j)
		{
			auto set = utils::getText(i, j, 2, greeting);
			s.Synth(pixelTime, set ? White : Black);
		}
	};

	for (auto i = 0; i < 16; ++i)
	{
		// sync pulse
		s.Synth(syncPulse, SyncPulse);

		textline(i);
		textline(i);
		textline(i);

		// separator
		s.Synth(syncPorch, SyncPorch);
	}
}