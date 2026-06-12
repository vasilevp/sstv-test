#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "bandpass_demodulator.hpp"
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
		std::println("  --prefilter biquad bandpass 1000..2400 Hz on the input");
		std::println("              (rejects mains hum and HF hiss; near no-op on");
		std::println("              clean recordings)");
		std::println("  --cadence-lock");
		std::println("              reject Schmitt sync events outside ±3 ms of");
		std::println("              the predicted next-line position and synthesise");
		std::println("              one if none arrives — keeps the picture aligned");
		std::println("              under noise where the sync pulse is lost or");
		std::println("              false positives fire mid-line");
	}
}

int main(int argc, char *argv[])
{
	// Extract flags from anywhere in argv; collect positional arguments.
	DemodKind demodKind = DemodKind::ZeroCrossing;
	bool prefilter = false;
	bool cadenceLock = false;
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
		else if (arg == "--prefilter")
		{
			prefilter = true;
		}
		else if (arg == "--cadence-lock")
		{
			cadenceLock = true;
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

		std::println("Demodulator: {}{}{}",
		             demodKindName(demodKind),
		             prefilter ? "  +bandpass" : "",
		             cadenceLock ? "  +cadence-lock" : "");

		// Construct a demodulator of the requested kind, optionally wrapped
		// in a bandpass to reject out-of-band noise on the input.
		auto makeDemod = [&]()
		{
			std::unique_ptr<Demodulator> d = makeDemodulator(demodKind, rate);
			if (prefilter)
				d = std::make_unique<BandpassDemodulator>(std::move(d));
			return d;
		};

		// Probe the VIS code from the start of the stream to choose the
		// matching decoder. A live receiver does the same: it listens until
		// a header arrives, then commits to a mode.
		auto probe = makeDemod();
		std::vector<float> probeFreq;
		size_t probeCount = std::min(samples.size(), size_t(rate) * 3);
		probeFreq.reserve(probeCount);
		for (size_t i = 0; i < probeCount; ++i)
			probeFreq.push_back(probe->process(samples[i]));
		Decoder::VIS vis = Decoder::detectVIS(probeFreq, rate);

		// Build a fresh demodulator (with prefilter if requested) for the
		// decoder to own. The probe above used its own instance; this one
		// is the stream-decoding demod.
		auto demod = makeDemod();

		std::unique_ptr<Decoder> decoder;
		switch (vis.found ? vis.code : 0)
		{
		// Robot Color
		case 8:
			decoder = std::make_unique<Robot36>(output, width, std::move(demod), cadenceLock);
			break;
		case 12:
			decoder = std::make_unique<Robot72>(output, width, std::move(demod), cadenceLock);
			break;

		// Martin: M3/M4 share per-line timing with M1/M2; they just send
		// fewer scanlines, which the streaming decoder counts dynamically.
		case 44:
		case 36:
			decoder = std::make_unique<Martin>(output, width, std::move(demod), 1, cadenceLock);
			break;
		case 40:
		case 32:
			decoder = std::make_unique<Martin>(output, width, std::move(demod), 2, cadenceLock);
			break;

		// Scottie: S3/S4 likewise share per-line timing with S1/S2.
		case 60:
		case 52:
			decoder = std::make_unique<Scottie>(output, width, std::move(demod), 138.240f, cadenceLock);
			break;
		case 56:
		case 48:
			decoder = std::make_unique<Scottie>(output, width, std::move(demod), 88.064f, cadenceLock);
			break;
		case 76:
			decoder = std::make_unique<Scottie>(output, width, std::move(demod), 345.600f, cadenceLock);
			break;

		// PD family. channelTime values back out from the handbook's stated
		// frame duration: pair = 22.08 ms (sync+porch) + 4 * channelTime,
		// frame = pair * (lines/2).
		case 93:
			decoder = std::make_unique<PD>(output, width, std::move(demod), 91.52f, cadenceLock);
			break;
		case 99:
			decoder = std::make_unique<PD>(output, width, std::move(demod), 170.24f, cadenceLock);
			break;
		case 95:
			decoder = std::make_unique<PD>(output, width, std::move(demod), 121.6f, cadenceLock);
			break;
		case 98:
			decoder = std::make_unique<PD>(output, width, std::move(demod), 195.584f, cadenceLock);
			break;
		case 96:
			decoder = std::make_unique<PD>(output, width, std::move(demod), 183.04f, cadenceLock);
			break;
		case 97:
			decoder = std::make_unique<PD>(output, width, std::move(demod), 244.48f, cadenceLock);
			break;
		case 94:
			decoder = std::make_unique<PD>(output, width, std::move(demod), 228.8f, cadenceLock);
			break;

		// Robot B&W 8 (VIS codes 1/2/3 — one per R/G/B filter component).
		case 1:
		case 2:
		case 3:
			decoder = std::make_unique<Robot8>(output, width, std::move(demod), cadenceLock);
			break;
		default:
			std::println("No decoder for VIS code {}; falling back to Robot 8 B/W",
			             vis.found ? std::to_string(vis.code) : std::string("(absent)"));
			decoder = std::make_unique<Robot8>(output, width, std::move(demod), cadenceLock);
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
