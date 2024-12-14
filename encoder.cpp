#include "encoder.hpp"

#include <stdexcept>

#include "LoadBMP/loadbmp.h"

using namespace std;

Encoder::Encoder(const string &input, const string &output, uint8_t visCode) : visCode(visCode), s(output)
{
	// load BMP
	auto err = loadbmp_decode_file(input.c_str(), &pixels, &width, &height, LOADBMP_RGB);

	if (err)
	{
		throw runtime_error("Error " + to_string(err));
	}
}

Encoder::~Encoder()
{
	free(pixels);
}

void Encoder::writeHeader()
{
	// sstv header
	// calibration pulse
	s.synth(300, Grey);
	s.synth(10, SyncPulse);
	s.synth(300, Grey);

	// VIS code
	// start/stop marker
	s.synth(30, SyncPulse);

	// 8 => 0b1000 => 0 0 0 1 0 0 0
	auto code = visCode;
	uint8_t parity = 0;
	for (auto i = 0; i < 7; i++)
	{
		s.synth(30, (code & 1) ? VISOne : VISZero);

		parity ^= code;
		code >>= 1;
	}

	parity ^= code;
	code >>= 1;

	// parity bit
	s.synth(30, (parity & 1) ? VISOne : VISZero);

	// start/stop marker
	s.synth(30, SyncPulse);
}