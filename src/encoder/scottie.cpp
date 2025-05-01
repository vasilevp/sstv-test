#include "scottie.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#include "encoder.hpp"
#include "synthesizer.hpp"
#include "utils.hpp"

Scottie::Scottie(
	// Input file name.
	const std::string &input,
	// Output synthesizer.
	Synthesizer &&s,
	// Mode.
	Mode mode,
	// Greeting text.
	const std::string &greeting) : Encoder(input, std::move(s), mode), greeting(greeting)
{
	utils::Guard();

	switch (mode)
	{
	case S1:
		lineTime = 138.240;
		break;
	case S2:
		lineTime = 88.064;
		break;
	case DX:
		lineTime = 345.600;
		break;
	default:
		throw std::invalid_argument("unknown mode");
	}
}

void Scottie::Encode()
{
	utils::Guard();

	if ((height % 120) != 0)
	{
		throw std::runtime_error("Image height must be divisible by 120!");
	}

	writeHeader();

	// sync pulse
	s.Synth(syncTime, SyncPulse);

	if (height < 256 || !greeting.empty())
	{
		writeGreeting();
	}

	for (uint32_t i = 0; i < height; i++)
	{
		colorLine(i, 1);
		colorLine(i, 2);

		// sync pulse
		s.Synth(syncTime, SyncPulse);

		colorLine(i, 0);
	}
}

void Scottie::colorLine(uint32_t i, size_t color)
{
	const float pixelTime = lineTime / float(width);

	// sync porch
	s.Synth(1.5, Frequency::SyncPorch);

	for (size_t j = 0; j < width; ++j)
	{
		size_t offset = (i * width + j) * 3;
		float c = pixels[offset + color];
		auto freq = Synthesizer::Lerp(c / 255);

		// pixel
		s.Synth(pixelTime, freq);
	}
};

void Scottie::writeGreeting()
{
	utils::Guard();

	const float pixelTime = lineTime / float(width);

	auto textLine = [&](auto i)
	{
		s.Synth(1.5, Frequency::SyncPorch);
		for (size_t j = 0; j < width; ++j)
		{
			auto set = utils::getText(i, j, 2, greeting);
			s.Synth(pixelTime, set ? White : Black);
		}
	};

	for (size_t i = 0; i < 16; ++i)
	{
		textLine(i);
		textLine(i);

		// sync pulse
		s.Synth(syncTime, SyncPulse);

		textLine(i);
	}
}