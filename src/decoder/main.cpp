#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "demodulator.hpp"
#include "martin.hpp"
#include "pd.hpp"
#include "robot36.hpp"
#include "robot72.hpp"
#include "robot8.hpp"
#include "scottie.hpp"
#include "wav.hpp"

namespace
{
	void printUsage(const char *argv0)
	{
		std::println("Usage: {} <input.wav> <output.bmp> [width] [--demod=zc|iq]", argv0);
		std::println("  Decodes an SSTV recording into a BMP. The mode is selected");
		std::println("  automatically from the header VIS code (Robot B&W 8, Robot");
		std::println("  Color 36/72, Martin M1..M4, Scottie S1..S4/DX, PD 50..290).");
		std::println("");
		std::println("  --demod=zc  zero-crossing demodulator (default; fast, ideal");
		std::println("              for clean synthetic recordings)");
		std::println("  --demod=iq  quadrature FM discriminator (more robust to");
		std::println("              noise; the design used by real SSTV decoders)");
	}
}

int main(int argc, char *argv[])
{
	// Extract --demod= from anywhere in argv; collect positional arguments.
	DemodKind demodKind = DemodKind::ZeroCrossing;
	std::vector<std::string> positional;
	for (int i = 1; i < argc; ++i)
	{
		std::string_view arg(argv[i]);
		if (arg.starts_with("--demod="))
		{
			std::string_view v = arg.substr(8);
			if (v == "zc" || v == "zero-crossing")
				demodKind = DemodKind::ZeroCrossing;
			else if (v == "iq" || v == "quadrature")
				demodKind = DemodKind::Quadrature;
			else
			{
				std::println("Error: --demod must be 'zc' or 'iq', got '{}'", v);
				return 1;
			}
		}
		else
		{
			positional.emplace_back(arg);
		}
	}

	if (positional.size() < 2 || positional.size() > 3)
	{
		printUsage(argv[0]);
		return 1;
	}

	try
	{
		const std::string input = positional[0];
		const std::string output = positional[1];
		uint32_t width = positional.size() == 3 ? uint32_t(std::stoul(positional[2])) : 320;

		WAVReader wav(input);
		const std::vector<float> &samples = wav.samples();
		const uint32_t rate = wav.sampleRate();
		if (samples.empty())
			throw std::runtime_error("Empty recording");

		std::println("Demodulator: {}", demodKindName(demodKind));

		// Probe the VIS code from the start of the stream to choose the
		// matching decoder. A live receiver does the same: it listens until
		// a header arrives, then commits to a mode.
		auto probe = makeDemodulator(demodKind, rate);
		std::vector<float> probeFreq;
		size_t probeCount = std::min(samples.size(), size_t(rate) * 3);
		probeFreq.reserve(probeCount);
		for (size_t i = 0; i < probeCount; ++i)
			probeFreq.push_back(probe->process(samples[i]));
		Decoder::VIS vis = Decoder::detectVIS(probeFreq, rate);

		std::unique_ptr<Decoder> decoder;
		switch (vis.found ? vis.code : 0)
		{
		// Robot Color
		case 8:
			decoder = std::make_unique<Robot36>(output, width, rate);
			break;
		case 12:
			decoder = std::make_unique<Robot72>(output, width, rate);
			break;

		// Martin: M3/M4 share per-line timing with M1/M2; they just send
		// fewer scanlines, which the streaming decoder counts dynamically.
		case 44:
		case 36:
			decoder = std::make_unique<Martin>(output, width, rate, 1);
			break;
		case 40:
		case 32:
			decoder = std::make_unique<Martin>(output, width, rate, 2);
			break;

		// Scottie: S3/S4 likewise share per-line timing with S1/S2.
		case 60:
		case 52:
			decoder = std::make_unique<Scottie>(output, width, rate, 138.240f);
			break;
		case 56:
		case 48:
			decoder = std::make_unique<Scottie>(output, width, rate, 88.064f);
			break;
		case 76:
			decoder = std::make_unique<Scottie>(output, width, rate, 345.600f);
			break;

		// PD family. channelTime values back out from the handbook's stated
		// frame duration: pair = 22.08 ms (sync+porch) + 4 * channelTime,
		// frame = pair * (lines/2).
		case 93:
			decoder = std::make_unique<PD>(output, width, rate, 91.52f);
			break;
		case 99:
			decoder = std::make_unique<PD>(output, width, rate, 170.24f);
			break;
		case 95:
			decoder = std::make_unique<PD>(output, width, rate, 121.6f);
			break;
		case 98:
			decoder = std::make_unique<PD>(output, width, rate, 195.584f);
			break;
		case 96:
			decoder = std::make_unique<PD>(output, width, rate, 183.04f);
			break;
		case 97:
			decoder = std::make_unique<PD>(output, width, rate, 244.48f);
			break;
		case 94:
			decoder = std::make_unique<PD>(output, width, rate, 228.8f);
			break;

		// Robot B&W 8 (VIS codes 1/2/3 — one per R/G/B filter component).
		case 1:
		case 2:
		case 3:
			decoder = std::make_unique<Robot8>(output, width, rate);
			break;
		default:
			std::println("No decoder for VIS code {}; falling back to Robot 8 B/W",
			             vis.found ? std::to_string(vis.code) : std::string("(absent)"));
			decoder = std::make_unique<Robot8>(output, width, rate);
			break;
		}

		decoder->setDemodKind(demodKind);

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
