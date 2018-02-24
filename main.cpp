#include <cstdint>
#include <iostream>
#include <functional>
#include <cmath>
#include <vector>

#include "utils.hpp"
#include "aprs.hpp"
#include "afsk.hpp"
#include "sstv.hpp"
#include "wav.hpp"

using namespace std::literals;

int main(int argc, char *argv[])
{
	const std::string usage = "Usage: "s + argv[0] + R"( <command> <arguments...>
Available commands:
	aprs <sender> <callsign> <target> <path> - sends APRS message
	sstv <input_bmp> <output_wav> - converts a BMP into ROBOT B/W 8 WAV SSTV format
)";

	if (argc < 4)
	{
		std::cerr << usage;
		return 1;
	}

	if (argv[1] == "aprs"s)
	{
		std::string message;
		for (int i = 6; i < argc; ++i)
		{
			message.append(argv[i]);
			message.push_back(' ');
		}

		auto packet = APRSPacket(
						  std::atoi(argv[2]),
						  argv[3],
						  std::atoi(argv[4]),
						  argv[5],
						  message)
						  .ToString();

		AFSK1200::Encode(packet, "aprs.wav");
	}
	else if (argv[1] == "sstv"s)
	{
		SSTV::Encode(argv[1], argv[2]);
	}
	else
	{
		std::cerr << usage;
		return 1;
	}

	return 0;
}
