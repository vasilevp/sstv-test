#include "scottie.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#include "encoder.hpp"
#include "synthesizer.hpp"
#include "utils.hpp"

Scottie::Scottie(
	RowSource &source,
	Synthesizer &&s,
	sstv::VisCode mode,
	const std::string &greeting) : Encoder(source, std::move(s), mode), greeting(greeting)
{
	utils::Guard();

	switch (mode)
	{
		using enum sstv::VisCode;
	case ScottieS1:
		lineTime = 138.240;
		standardLines = 256;
		break;
	case ScottieS2:
		lineTime = 88.064;
		standardLines = 256;
		break;
	case ScottieS3:
		// S3 has the same per-line timing as S1, but only 128 image lines.
		lineTime = 138.240;
		standardLines = 128;
		break;
	case ScottieS4:
		// S4 has the same per-line timing as S2, but only 128 image lines.
		lineTime = 88.064;
		standardLines = 128;
		break;
	case ScottieDX:
		lineTime = 345.600;
		standardLines = 256;
		break;
	default:
		throw std::invalid_argument("unknown mode");
	}
}

void Scottie::Encode()
{
	utils::Guard();

	writeHeader();

	// sync pulse
	s.Synth(syncTime, SyncPulse);

	if (height < standardLines || !greeting.empty())
	{
		writeGreeting();
	}

	const uint32_t lines = std::min(height, standardLines);
	for (uint32_t i = 0; i < lines; i++)
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

	const auto row = source.row(i);
	for (size_t j = 0; j < width; ++j)
	{
		float c = row[j * 3 + color];
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