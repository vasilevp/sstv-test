#include <cstdint>
#include <iostream>
#include <functional>
#include <cmath>
#include <vector>

#include "utils.hpp"
#include "robot8.hpp"
#include "robot36.hpp"
#include "martin.hpp"
#include "wav.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		println("Usage: {} <input_bmp> - converts a BMP into ROBOT B/W 8 WAV SSTV format", argv[0]);
		return 1;
	}

	Robot8(argv[1], "outputs/robot8.wav").Encode();
	Robot(argv[1], "outputs/robot36.wav").Encode();
	Martin(argv[1], "outputs/martin1.wav", 1).Encode();
	Martin(argv[1], "outputs/martin2.wav", 2).Encode();
	return 0;
}
