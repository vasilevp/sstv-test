#include "pd.hpp"

#include <cstddef>
#include <stdexcept>

#include "synthesizer.hpp"
#include "utils.hpp"

void PD::Encode()
{
	utils::Guard();

	if (height % 2 != 0)
		throw std::runtime_error("PD requires an even image height (lines are sent in pairs)");

	writeHeader();
	if (!greeting.empty())
		writeGreeting();

	const float pixelTime = channelTime / float(width);

	for (size_t i = 0; i < height; i += 2)
	{
		// Sync and porch for the line pair.
		s.Synth(syncPulse, SyncPulse);
		s.Synth(syncPorch, SyncPorch);

		// Y of line i.
		for (size_t j = 0; j < width; ++j)
		{
			const size_t offset = (i * width + j) * 3;
			const float Y = getY(pixels, offset);
			s.Synth(pixelTime, Synthesizer::Lerp(Y / 255));
		}

		// R-Y averaged across the pair.
		for (size_t j = 0; j < width; ++j)
		{
			const float c1 = getChromaRed(pixels, (i * width + j) * 3);
			const float c2 = getChromaRed(pixels, ((i + 1) * width + j) * 3);
			s.Synth(pixelTime, Synthesizer::Lerp((c1 + c2) / 2 / 255));
		}

		// B-Y averaged across the pair.
		for (size_t j = 0; j < width; ++j)
		{
			const float c1 = GetChromaBlue(pixels, (i * width + j) * 3);
			const float c2 = GetChromaBlue(pixels, ((i + 1) * width + j) * 3);
			s.Synth(pixelTime, Synthesizer::Lerp((c1 + c2) / 2 / 255));
		}

		// Y of line i+1.
		for (size_t j = 0; j < width; ++j)
		{
			const size_t offset = ((i + 1) * width + j) * 3;
			const float Y = getY(pixels, offset);
			s.Synth(pixelTime, Synthesizer::Lerp(Y / 255));
		}
	}
}

void PD::writeGreeting()
{
	utils::Guard();

	const float pixelTime = channelTime / float(width);

	// 8 pairs = 16 luminance rows, enough for one 16-px-tall row of 8x8
	// font scaled 2x. Chroma is sent neutral so the greeting reads as plain
	// white-on-black text.
	for (size_t i = 0; i < 16; i += 2)
	{
		s.Synth(syncPulse, SyncPulse);
		s.Synth(syncPorch, SyncPorch);

		auto textRow = [&](size_t row)
		{
			for (size_t j = 0; j < width; ++j)
			{
				const auto set = utils::getText(row, j, 2, greeting);
				s.Synth(pixelTime, set ? White : Black);
			}
		};

		textRow(i);     // Y_a
		// R-Y neutral
		for (size_t j = 0; j < width; ++j)
			s.Synth(pixelTime, Grey);
		// B-Y neutral
		for (size_t j = 0; j < width; ++j)
			s.Synth(pixelTime, Grey);
		textRow(i + 1); // Y_b
	}
}
