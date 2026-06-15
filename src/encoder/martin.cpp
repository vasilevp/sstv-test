#include "martin.hpp"
#include "synthesizer.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>

using namespace std;

void Martin::Encode()
{
	utils::Guard();

	const uint32_t expected = standardLines(mode);
	if (height < expected)
	{
		std::print(cerr,
				   "WARN: Image height is {} px but Martin M{} expects {}; "
				   "the transmission will be a non-standard short variant.\n",
				   height, mode, expected);
	}

	writeHeader();
	if (!greeting.empty())
		writeGreeting();

	const uint32_t lines = std::min(height, expected);
	for (uint32_t i = 0; i < lines; ++i)
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
	const float pixelTime = lineTime * float(channelMultiplier(mode)) / float(width);

	// sync porch
	s.Synth(syncPorch, Frequency::SyncPorch);

	const auto row = source.row(i);
	for (size_t j = 0; j < width; ++j)
	{
		const float c = row[j * 3 + color];
		auto freq = Synthesizer::Lerp(c / 255);

		// pixel
		s.Synth(pixelTime, freq);
	}
};

void Martin::writeGreeting()
{
	utils::Guard();

	const float pixelTime = lineTime * float(channelMultiplier(mode)) / float(width);

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
