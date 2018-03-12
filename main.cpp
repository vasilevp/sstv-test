#include <cstdint>
#include <iostream>
#include <functional>
#include <cmath>
#include <vector>

#include "utils.hpp"
#include "sstv.hpp"
#include "wav.hpp"

using namespace std::literals;

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr
		    << "Usage: " << argv[0] << " <input_bmp> <output_wav> - converts a BMP into ROBOT B/W 8 WAV SSTV format" << std::endl;
		return 1;
	}

	SSTV::Encode(argv[1], argv[2]);
	return 0;
}
