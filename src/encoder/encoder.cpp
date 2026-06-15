#include "encoder.hpp"

#include "synthesizer.hpp"
#include "utils.hpp"

using namespace std;

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
