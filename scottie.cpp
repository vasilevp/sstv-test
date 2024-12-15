#include "scottie.hpp"

#include <stdexcept>
#include <cstdint>

#include "utils.hpp"

Scottie::Scottie(
	// Input file name.
	const std::string &input,
	// Output file name.
	const std::string &output,
	// Mode.
	Mode mode,
	// Greeting text.
	const std::string &greeting) : Encoder(input, output, mode), greeting(greeting)
{
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
};

void Scottie::Encode()
{
	if ((height % 120) != 0)
	{
		throw std::runtime_error("Image height must be divisible by 120!");
	}

	writeHeader();

	// sync pulse
	s.synth(syncTime, SyncPulse);

	if (height < 256 || !greeting.empty())
	{
		writeGreeting();
	}

	const float pixelTime = lineTime / width;

	for (size_t i = 0; i < height; i++)
	{
		colorLine(i, 1);
		colorLine(i, 2);

		// sync pulse
		s.synth(syncTime, SyncPulse);

		colorLine(i, 0);
	}
}

void Scottie::colorLine(int i, size_t color)
{
	const float pixelTime = lineTime / width;

	// sync porch
	s.synth(1.5, Frequency::SyncPorch);

	for (size_t j = 0; j < width; ++j)
	{
		size_t offset = (i * width + j) * 3;
		float c = pixels[offset + color];
		auto freq = Synthesizer::Lerp(c / 255);

		// pixel
		s.synth(pixelTime, freq);
	}
};

void Scottie::writeGreeting()
{
	const float pixelTime = lineTime / width;

	auto textLine = [&](auto i)
	{
		s.synth(1.5, Frequency::SyncPorch);
		for (size_t j = 0; j < width; ++j)
		{
			auto set = utils::getText(i, j, 2, greeting);
			s.synth(pixelTime, set ? White : Black);
		}
	};

	for (size_t i = 0; i < 16; ++i)
	{
		textLine(i);
		textLine(i);

		// sync pulse
		s.synth(syncTime, SyncPulse);

		textLine(i);
	}
}