#include "martin.hpp"

#include <stdexcept>
#include <cstdint>
#include <iostream>
#include <print>

#include "utils.hpp"

#include "LoadBMP/loadbmp.h"

using namespace std;

void Martin::Encode()
{
	if (height != 256)
	{
		print(cerr, "WARN: Image height must be 256 pixels! Provided height of {} may cause issues with the receiver.", height);
	}

	writeHeader();
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

	// padding for reference
	for (size_t i = height; i < 256; ++i)
	{
		// sync pulse
		s.synth(syncPulse, SyncPulse);

		colorLine(height - 1, 1);
		colorLine(height - 1, 2);
		colorLine(height - 1, 0);

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