#include <cstdint>
#include <exception>
#include <print>
#include <string>

#include "robot8.hpp"

int main(int argc, char *argv[])
{
	if (argc < 3 || argc > 4)
	{
		std::println("Usage: {} <input.wav> <output.bmp> [width]", argv[0]);
		std::println("  Decodes a Robot 8 B/W SSTV recording into a BMP.");
		std::println("  width defaults to 320 (the encoder's test image width).");
		return 1;
	}

	try
	{
		uint32_t width = argc == 4 ? uint32_t(std::stoul(argv[3])) : 320;
		Robot8(argv[1], argv[2], width).Decode();
	}
	catch (const std::exception &e)
	{
		std::println("Error: {}", e.what());
		return 1;
	}
	return 0;
}
