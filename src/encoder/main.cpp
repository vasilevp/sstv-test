#include <exception>
#include <print>

#include "martin.hpp"
#include "pd.hpp"
#include "robot36.hpp"
#include "robot72.hpp"
#include "robot8.hpp"
#include "scottie.hpp"
#include "synthesizer.hpp"
#include "utils.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	utils::Guard();

	if (argc != 2)
	{
		println("Usage: {} <input_bmp> - converts a BMP into ROBOT B/W 8 WAV SSTV format", argv[0]);
		return 1;
	}

	try
	{
		Robot8(argv[1], "outputs/raw.wav", "raw 5 93 9", 5, 93, 9).Encode();

		Scottie(argv[1], "outputs/scottie1.wav", Scottie::S1, "Scottie S1").Encode();
		Scottie(argv[1], "outputs/scottie2.wav", Scottie::S2, "Scottie S2").Encode();
		Scottie(argv[1], "outputs/scottie3.wav", Scottie::S3, "Scottie S3").Encode();
		Scottie(argv[1], "outputs/scottie4.wav", Scottie::S4, "Scottie S4").Encode();
		Scottie(argv[1], "outputs/scottieDX.wav", Scottie::DX, "Scottie DX").Encode();

		Robot8(argv[1], "outputs/robot8.wav", "Robot 8").Encode();
		Robot36(argv[1], "outputs/robot36.wav", "Robot 36").Encode();
		Robot72(argv[1], "outputs/robot72.wav", "Robot 72").Encode();

		Martin(argv[1], "outputs/martin1.wav", 1, "Martin M1").Encode();
		Martin(argv[1], "outputs/martin2.wav", 2, "Martin M2").Encode();
		Martin(argv[1], "outputs/martin3.wav", 3, "Martin M3").Encode();
		Martin(argv[1], "outputs/martin4.wav", 4, "Martin M4").Encode();

		// PD family. channelTime per Bruchanov (sstv-handbook.com), ch. 5;
		// derived from each mode's frame duration so a pair (sync 20 ms +
		// porch 2.08 ms + 4 * channelTime) times pairs equals the spec
		// duration. Pixel time is channelTime / width, so a non-standard
		// input width keeps the channel duration intact but stretches the
		// per-pixel timing.
		PD(argv[1], "outputs/pd50.wav",   91.52f,  93, "PD 50").Encode();
		PD(argv[1], "outputs/pd90.wav",  170.24f,  99, "PD 90").Encode();
		PD(argv[1], "outputs/pd120.wav", 121.6f,   95, "PD 120").Encode();
		PD(argv[1], "outputs/pd160.wav", 195.584f, 98, "PD 160").Encode();
		PD(argv[1], "outputs/pd180.wav", 183.04f,  96, "PD 180").Encode();
		PD(argv[1], "outputs/pd240.wav", 244.48f,  97, "PD 240").Encode();
		PD(argv[1], "outputs/pd290.wav", 228.8f,   94, "PD 290").Encode();
	}
	catch (const std::exception &e)
	{
		println("Error: {}", e.what());
		return 1;
	}
	return 0;
}
