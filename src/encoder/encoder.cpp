#include "encoder.hpp"
#include "synthesizer.hpp"
#include "utils.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

#include <LoadBMP/loadbmp.h>

using namespace std;

Encoder::Encoder(const string &input, Synthesizer &&s, uint8_t visCode) : visCode(visCode), s(std::move(s)), pixels(nullptr)
{
	utils::Guard();

	// load BMP
	auto err = loadbmp_decode_file(input.c_str(), &pixels, &width, &height, LOADBMP_RGB);

	if (err)
	{
		throw runtime_error("Error " + to_string(err));
	}
}

Encoder::~Encoder()
{
	utils::Guard();

	if (pixels)
	{
		free(pixels);
		pixels = nullptr;
	}
}

void Encoder::writeHeader()
{
	utils::Guard();

	// sstv header
	// calibration pulse
	s.Synth(300, Grey);
	s.Synth(10, SyncPulse);
	s.Synth(300, Grey);

	// VIS code
	// start/stop marker
	s.Synth(30, SyncPulse);

	// 8 => 0b1000 => 0 0 0 1 0 0 0
	auto code = visCode;
	uint8_t parity = 0;
	for (auto i = 0; i < 7; i++)
	{
		s.Synth(30, (code & 1) ? VISOne : VISZero);

		parity ^= code;
		code >>= 1;
	}

	parity ^= code;

	// parity bit
	s.Synth(30, (parity & 1) ? VISOne : VISZero);

	// start/stop marker
	s.Synth(30, SyncPulse);
}