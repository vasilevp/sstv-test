#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <print>
#include <span>
#include <string>
#include <vector>

#include "demodulator.hpp"
#include "martin.hpp"
#include "pd.hpp"
#include "robot36.hpp"
#include "robot72.hpp"
#include "robot8.hpp"
#include "scottie.hpp"
#include "wav.hpp"

int main(int argc, char *argv[])
{
	if (argc < 3 || argc > 4)
	{
		std::println("Usage: {} <input.wav> <output.bmp> [width]", argv[0]);
		std::println("  Decodes an SSTV recording into a BMP. The mode is selected");
		std::println("  automatically from the header VIS code (Robot 8 B/W,");
		std::println("  Robot 36, Robot 72).");
		return 1;
	}

	try
	{
		const std::string input = argv[1];
		const std::string output = argv[2];
		uint32_t width = argc == 4 ? uint32_t(std::stoul(argv[3])) : 320;

		WAVReader wav(input);
		const std::vector<float> &samples = wav.samples();
		const uint32_t rate = wav.sampleRate();
		if (samples.empty())
			throw std::runtime_error("Empty recording");

		// Probe the VIS code from the start of the stream to choose the
		// matching decoder. A live receiver does the same: it listens until
		// a header arrives, then commits to a mode.
		Demodulator probe(rate);
		std::vector<float> probeFreq;
		size_t probeCount = std::min(samples.size(), size_t(rate) * 3);
		probeFreq.reserve(probeCount);
		for (size_t i = 0; i < probeCount; ++i)
			probeFreq.push_back(probe.process(samples[i]));
		Decoder::VIS vis = Decoder::detectVIS(probeFreq, rate);

		std::unique_ptr<Decoder> decoder;
		switch (vis.found ? vis.code : 0)
		{
		case 8:
			decoder = std::make_unique<Robot36>(output, width, rate);
			break;
		case 12:
			decoder = std::make_unique<Robot72>(output, width, rate);
			break;
		case 44:
			decoder = std::make_unique<Martin>(output, width, rate, 1);
			break;
		case 40:
			decoder = std::make_unique<Martin>(output, width, rate, 2);
			break;
		case 60:
			decoder = std::make_unique<Scottie>(output, width, rate, 138.240f);
			break;
		case 56:
			decoder = std::make_unique<Scottie>(output, width, rate, 88.064f);
			break;
		case 76:
			decoder = std::make_unique<Scottie>(output, width, rate, 345.600f);
			break;
		case 95:
			decoder = std::make_unique<PD>(output, width, rate, 121.6f);
			break;
		case 1:
			decoder = std::make_unique<Robot8>(output, width, rate);
			break;
		default:
			std::println("No decoder for VIS code {}; falling back to Robot 8 B/W",
			             vis.found ? std::to_string(vis.code) : std::string("(absent)"));
			decoder = std::make_unique<Robot8>(output, width, rate);
			break;
		}

		// Feed the recording to the decoder in fixed-size blocks, the way a
		// live audio source would deliver it.
		constexpr size_t Block = 4096;
		for (size_t i = 0; i < samples.size(); i += Block)
		{
			size_t n = std::min(Block, samples.size() - i);
			decoder->feed(std::span<const float>(samples.data() + i, n));
		}
		decoder->finish();
	}
	catch (const std::exception &e)
	{
		std::println("Error: {}", e.what());
		return 1;
	}
	return 0;
}
