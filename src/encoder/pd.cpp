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

		// Y of line i. The PixelSource's two-row LRU keeps row i resident
		// across the chroma loops below, so the alternating row(i)/row(i+1)
		// accesses don't re-seek.
		{
			const auto rowA = pixels.row(i);
			for (size_t j = 0; j < width; ++j)
			{
				const float Y = getY(rowA.subspan(j * 3, 3));
				s.Synth(pixelTime, Synthesizer::Lerp(Y / 255));
			}
		}

		// R-Y averaged across the pair.
		for (size_t j = 0; j < width; ++j)
		{
			const float c1 = getChromaRed(pixels.row(i).subspan(j * 3, 3));
			const float c2 = getChromaRed(pixels.row(i + 1).subspan(j * 3, 3));
			s.Synth(pixelTime, Synthesizer::Lerp((c1 + c2) / 2 / 255));
		}

		// B-Y averaged across the pair.
		for (size_t j = 0; j < width; ++j)
		{
			const float c1 = GetChromaBlue(pixels.row(i).subspan(j * 3, 3));
			const float c2 = GetChromaBlue(pixels.row(i + 1).subspan(j * 3, 3));
			s.Synth(pixelTime, Synthesizer::Lerp((c1 + c2) / 2 / 255));
		}

		// Y of line i+1.
		{
			const auto rowB = pixels.row(i + 1);
			for (size_t j = 0; j < width; ++j)
			{
				const float Y = getY(rowB.subspan(j * 3, 3));
				s.Synth(pixelTime, Synthesizer::Lerp(Y / 255));
			}
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
