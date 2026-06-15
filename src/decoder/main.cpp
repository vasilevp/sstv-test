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
#include "bmp_row_sink.hpp"
#include "demodulator.hpp"
#include "martin.hpp"
#include "pd.hpp"
#include "robot36.hpp"
#include "robot72.hpp"
#include "robot8.hpp"
#include "row_sink.hpp"
#include "sample_source.hpp"
#include "scottie.hpp"
#include "vis_detector.hpp"
#include "wav_sample_source.hpp"

namespace
{
	void printUsage(const char *argv0)
	{
		std::println("Usage: {} <input.wav> <output.bmp> [--width=N] [--demod=zc|iq]", argv0);
		std::println("  Decodes an SSTV recording into a BMP. The mode is selected");
		std::println("  automatically from the header VIS code (Robot B&W 8, Robot");
		std::println("  Color 36/72, Martin M1..M4, Scottie S1..S4/DX, PD 50..290).");
		std::println("");
		std::println("  --width=N   output pixels per line; defaults to the detected");
		std::println("              mode's native grid (320 for most, 640 for PD120/");
		std::println("              180/240, 512 for PD160, 800 for PD290, 160 for");
		std::println("              Robot 8). Pass a value to override.");
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
	DemodKind demodKind = DemodKind::ZeroCrossing;
	bool prefilter = false;
	bool cadenceLock = false;
	bool widthOverride = false;
	uint32_t cliWidth = 0;
	std::vector<std::string> positional;
	for (int i = 1; i < argc; ++i)
	{
		std::string_view arg(argv[i]);
		if (arg.starts_with("--width="))
		{
			std::string_view v = arg.substr(8);
			try
			{
				cliWidth = uint32_t(std::stoul(std::string(v)));
			}
			catch (const std::exception &)
			{
				cliWidth = 0;
			}
			if (cliWidth == 0)
			{
				std::println("Error: --width must be a positive integer, got '{}'", v);
				return 1;
			}
			widthOverride = true;
		}
		else if (arg.starts_with("--demod="))
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

	if (positional.size() != 2)
	{
		printUsage(argv[0]);
		return 1;
	}

	try
	{
		const std::string input = positional[0];
		const std::string output = positional[1];
		// --width overrides the mode's native grid; otherwise width is chosen
		// from the detected VIS code below.

		std::unique_ptr<SampleSource> source = std::make_unique<WAVSampleSource>(input);
		const std::uint32_t rate = source->sampleRate();

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

		// Pull audio from the streaming source into a reusable buffer and probe
		// it for the VIS header sample-by-sample through a streaming detector,
		// stopping as soon as the header is located. Real off-air recordings
		// carry several seconds of voice and static before the SSTV header (the
		// ISS/ARISS recordings run up to ~5.6 s), so there is no search
		// timeout — we read until a header turns up or the stream ends. The
		// buffer is replayed into the real decoder below, so no input is lost.
		// A live receiver behaves the same: it listens until a header arrives,
		// then commits to a mode.
		constexpr std::size_t Block = 4096;
		std::vector<float> probeAudio;
		std::vector<float> blockBuf(Block);
		auto probe = makeDemod();
		VisDetector probeDetector(rate);
		Decoder::VIS vis;
		while (!vis.found)
		{
			const std::size_t n = source->read(blockBuf);
			if (n == 0)
				break;
			for (std::size_t i = 0; i < n; ++i)
			{
				probeAudio.push_back(blockBuf[i]);
				if (probeDetector.update(probe->process(blockBuf[i])))
				{
					vis = probeDetector.result();
					break;
				}
			}
		}
		if (probeAudio.empty())
			throw std::runtime_error("Empty recording");

		// Build the decoder's own demodulator and row sink. The probe used
		// its own throwaway demodulator; this one will see every sample of
		// the recording, including the probe section replayed back below.
		auto demod = makeDemod();

		// Width follows the detected mode's native pixel grid unless the user
		// forced one. This is what keeps PD120 (ISS) and the other wide PD
		// modes from decoding at the 320 default and coming out vertically
		// stretched — the row count is fixed by the transmission, so an
		// under-wide grid squashes the aspect ratio.
		const uint32_t width = widthOverride
		                           ? cliWidth
		                           : sstv::visModeWidth(vis.found ? vis.code : 0);

		std::unique_ptr<RowSink> sink = std::make_unique<BMPRowSink>(output, width);

		std::unique_ptr<Decoder> decoder;
		switch (vis.found ? vis.code : 0)
		{
		// Robot Color
		case 8:
			decoder = std::make_unique<Robot36>(std::move(sink), width, std::move(demod), cadenceLock);
			break;
		case 12:
			decoder = std::make_unique<Robot72>(std::move(sink), width, std::move(demod), cadenceLock);
			break;

		// Martin: M3/M4 share per-line timing with M1/M2; they just send
		// fewer scanlines, which the streaming decoder counts dynamically.
		case 44:
		case 36:
			decoder = std::make_unique<Martin>(std::move(sink), width, std::move(demod), 1, cadenceLock);
			break;
		case 40:
		case 32:
			decoder = std::make_unique<Martin>(std::move(sink), width, std::move(demod), 2, cadenceLock);
			break;

		// Scottie: S3/S4 likewise share per-line timing with S1/S2.
		case 60:
		case 52:
			decoder = std::make_unique<Scottie>(std::move(sink), width, std::move(demod), 138.240f, cadenceLock);
			break;
		case 56:
		case 48:
			decoder = std::make_unique<Scottie>(std::move(sink), width, std::move(demod), 88.064f, cadenceLock);
			break;
		case 76:
			decoder = std::make_unique<Scottie>(std::move(sink), width, std::move(demod), 345.600f, cadenceLock);
			break;

		// PD family. channelTime values back out from the handbook's stated
		// frame duration: pair = 22.08 ms (sync+porch) + 4 * channelTime,
		// frame = pair * (lines/2).
		case 93:
			decoder = std::make_unique<PD>(std::move(sink), width, std::move(demod), 91.52f, cadenceLock);
			break;
		case 99:
			decoder = std::make_unique<PD>(std::move(sink), width, std::move(demod), 170.24f, cadenceLock);
			break;
		case 95:
			decoder = std::make_unique<PD>(std::move(sink), width, std::move(demod), 121.6f, cadenceLock);
			break;
		case 98:
			decoder = std::make_unique<PD>(std::move(sink), width, std::move(demod), 195.584f, cadenceLock);
			break;
		case 96:
			decoder = std::make_unique<PD>(std::move(sink), width, std::move(demod), 183.04f, cadenceLock);
			break;
		case 97:
			decoder = std::make_unique<PD>(std::move(sink), width, std::move(demod), 244.48f, cadenceLock);
			break;
		case 94:
			decoder = std::make_unique<PD>(std::move(sink), width, std::move(demod), 228.8f, cadenceLock);
			break;

		// Robot B&W 8 (VIS codes 1/2/3 — one per R/G/B filter component).
		case 1:
		case 2:
		case 3:
			decoder = std::make_unique<Robot8>(std::move(sink), width, std::move(demod), cadenceLock);
			break;
		default:
			std::println("No decoder for VIS code {}; falling back to Robot 8 B/W",
			             vis.found ? std::to_string(vis.code) : std::string("(absent)"));
			decoder = std::make_unique<Robot8>(std::move(sink), width, std::move(demod), cadenceLock);
			break;
		}

		// Replay the probe audio into the real decoder, then keep pulling
		// blocks from the streaming source until EOF — together these cover
		// the entire recording in one pass without ever materialising it
		// all in memory.
		for (std::size_t i = 0; i < probeAudio.size(); i += Block)
		{
			const std::size_t n = std::min(Block, probeAudio.size() - i);
			decoder->feed(std::span<const float>(probeAudio.data() + i, n));
		}
		while (true)
		{
			const std::size_t n = source->read(blockBuf);
			if (n == 0)
				break;
			decoder->feed(std::span<const float>(blockBuf.data(), n));
		}
		decoder->finish();
		std::println("Wrote {}", output);
	}
	catch (const std::exception &e)
	{
		std::println("Error: {}", e.what());
		return 1;
	}
	return 0;
}
