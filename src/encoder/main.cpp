#include <exception>
#include <print>
#include <string>

#include "bmp_row_source.hpp"
#include "martin.hpp"
#include "pd.hpp"
#include "robot36.hpp"
#include "robot72.hpp"
#include "robot8.hpp"
#include "scottie.hpp"
#include "synthesizer.hpp"
#include "utils.hpp"
#include "wav_sample_sink.hpp"

using namespace std;

namespace
{
	// Tie a WAVSampleSink + Synthesizer together over an output path. The
	// sink stays alive for as long as we hold the helper, the synthesizer
	// references it. After the encode is driven to completion, finish()
	// closes the sink (patching the RIFF + data sizes in the header).
	struct Run
	{
		WAVSampleSink sink;
		Synthesizer synth;

		Run(const string &out, std::uint32_t rate = 8000)
			: sink(out, rate), synth(sink, rate)
		{
		}

		void finish() { sink.finish(); }
	};
}

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
		// The pixel source is opened once and reused across every encode;
		// its tiny two-row cache makes top-down access cheap regardless of
		// how many output modes we run.
		BMPRowSource source(argv[1]);

		{
			Run r("outputs/raw.wav");
			Robot8(source, std::move(r.synth), "raw 5 93 9", 5, 93, 9).Encode();
			r.finish();
		}

		{
			Run r("outputs/scottie1.wav");
			Scottie(source, std::move(r.synth), Scottie::S1, "Scottie S1").Encode();
			r.finish();
		}
		{
			Run r("outputs/scottie2.wav");
			Scottie(source, std::move(r.synth), Scottie::S2, "Scottie S2").Encode();
			r.finish();
		}
		{
			Run r("outputs/scottie3.wav");
			Scottie(source, std::move(r.synth), Scottie::S3, "Scottie S3").Encode();
			r.finish();
		}
		{
			Run r("outputs/scottie4.wav");
			Scottie(source, std::move(r.synth), Scottie::S4, "Scottie S4").Encode();
			r.finish();
		}
		{
			Run r("outputs/scottieDX.wav");
			Scottie(source, std::move(r.synth), Scottie::DX, "Scottie DX").Encode();
			r.finish();
		}

		{
			Run r("outputs/robot8.wav");
			Robot8(source, std::move(r.synth), "Robot 8").Encode();
			r.finish();
		}
		{
			Run r("outputs/robot36.wav");
			Robot36(source, std::move(r.synth), "Robot 36").Encode();
			r.finish();
		}
		{
			Run r("outputs/robot72.wav");
			Robot72(source, std::move(r.synth), "Robot 72").Encode();
			r.finish();
		}

		{
			Run r("outputs/martin1.wav");
			Martin(source, std::move(r.synth), 1, "Martin M1").Encode();
			r.finish();
		}
		{
			Run r("outputs/martin2.wav");
			Martin(source, std::move(r.synth), 2, "Martin M2").Encode();
			r.finish();
		}
		{
			Run r("outputs/martin3.wav");
			Martin(source, std::move(r.synth), 3, "Martin M3").Encode();
			r.finish();
		}
		{
			Run r("outputs/martin4.wav");
			Martin(source, std::move(r.synth), 4, "Martin M4").Encode();
			r.finish();
		}

		// PD family. channelTime per Bruchanov (sstv-handbook.com), ch. 5;
		// derived from each mode's frame duration so a pair (sync 20 ms +
		// porch 2.08 ms + 4 * channelTime) times pairs equals the spec
		// duration. Pixel time is channelTime / width, so a non-standard
		// input width keeps the channel duration intact but stretches the
		// per-pixel timing.
		{
			Run r("outputs/pd50.wav");
			PD(source, std::move(r.synth), 91.52f, 93, "PD 50").Encode();
			r.finish();
		}
		{
			Run r("outputs/pd90.wav");
			PD(source, std::move(r.synth), 170.24f, 99, "PD 90").Encode();
			r.finish();
		}
		{
			Run r("outputs/pd120.wav");
			PD(source, std::move(r.synth), 121.6f, 95, "PD 120").Encode();
			r.finish();
		}
		{
			Run r("outputs/pd160.wav");
			PD(source, std::move(r.synth), 195.584f, 98, "PD 160").Encode();
			r.finish();
		}
		{
			Run r("outputs/pd180.wav");
			PD(source, std::move(r.synth), 183.04f, 96, "PD 180").Encode();
			r.finish();
		}
		{
			Run r("outputs/pd240.wav");
			PD(source, std::move(r.synth), 244.48f, 97, "PD 240").Encode();
			r.finish();
		}
		{
			Run r("outputs/pd290.wav");
			PD(source, std::move(r.synth), 228.8f, 94, "PD 290").Encode();
			r.finish();
		}
	}
	catch (const std::exception &e)
	{
		println("Error: {}", e.what());
		return 1;
	}
	return 0;
}
