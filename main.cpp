#include <cstdint>
#include <iostream>
#include <functional>
#include <cmath>
#include <vector>

#include "robot8.hpp"
#include "robot36.hpp"
#include "robot72.hpp"
#include "martin.hpp"
#include "scottie.hpp"
#include "wav.hpp"
#include "synthesizer.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		println("Usage: {} <input_bmp> - converts a BMP into ROBOT B/W 8 WAV SSTV format", argv[0]);
		return 1;
	}

	Robot8(argv[1], "outputs/raw.wav", "raw 5 93 9", 5, 93, 9).Encode();

	Scottie(argv[1], "outputs/scottie1.wav", Scottie::S1, "Scottie 1").Encode();
	Scottie(argv[1], "outputs/scottie2.wav", Scottie::S2, "Scottie 2").Encode();
	Scottie(argv[1], "outputs/scottieDX.wav", Scottie::DX, "Scottie DX").Encode();

	Robot8(argv[1], "outputs/robot8.wav", "Robot 8").Encode();
	Robot36(argv[1], "outputs/robot36.wav", "Robot 36").Encode();
	Robot72(argv[1], "outputs/robot72.wav", "Robot 72").Encode();

	Martin(argv[1], "outputs/martin1.wav", 1, "Martin 1").Encode();
	Martin(argv[1], "outputs/martin2.wav", 2, "Martin 2").Encode();
	return 0;
}
