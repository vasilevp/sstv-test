#include <cstdint>
#include <exception>
#include <memory>
#include <print>
#include <string>

#include "demodulator.hpp"
#include "robot36.hpp"
#include "robot72.hpp"
#include "robot8.hpp"

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

		// Probe the header VIS code, then dispatch to the matching decoder.
		Decoder::VIS vis = Decoder::detectVIS(Demodulator(input));

		std::unique_ptr<Decoder> decoder;
		switch (vis.found ? vis.code : 0)
		{
		case 8:
			decoder = std::make_unique<Robot36>(input, output, width);
			break;
		case 12:
			decoder = std::make_unique<Robot72>(input, output, width);
			break;
		case 1:
			decoder = std::make_unique<Robot8>(input, output, width);
			break;
		default:
			std::println("No decoder for VIS code {}; falling back to Robot 8 B/W",
			             vis.found ? std::to_string(vis.code) : std::string("(absent)"));
			decoder = std::make_unique<Robot8>(input, output, width);
			break;
		}

		decoder->Decode();
	}
	catch (const std::exception &e)
	{
		std::println("Error: {}", e.what());
		return 1;
	}
	return 0;
}
