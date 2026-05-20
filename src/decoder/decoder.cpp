#include "decoder.hpp"

#include <bit>

// Maps the VIS codes this project's encoder emits to mode names. Codes
// 8/12/40/44/56/60/76 are the standard SSTV assignments; code 1 is the
// non-standard value the encoder uses for its Robot 8 B/W mode.
const char *sstv::visModeName(uint8_t code)
{
	switch (code)
	{
	case 1:  return "Robot 8 B/W";
	case 8:  return "Robot 36";
	case 12: return "Robot 72";
	case 40: return "Martin 2";
	case 44: return "Martin 1";
	case 56: return "Scottie 2";
	case 60: return "Scottie 1";
	case 76: return "Scottie DX";
	default: return "unknown";
	}
}

std::vector<Decoder::Run> Decoder::syncRuns() const
{
	std::vector<Run> runs;
	for (size_t i = 0; i < demod.size();)
	{
		float f = demod.at(i);
		if (f > 0.0f && f < sstv::SyncThreshold)
		{
			size_t begin = i;
			while (i < demod.size())
			{
				float g = demod.at(i);
				if (!(g > 0.0f && g < sstv::SyncThreshold))
					break;
				++i;
			}
			runs.push_back({begin, i});
		}
		else
		{
			++i;
		}
	}
	return runs;
}

Decoder::VIS Decoder::detectVIS() const
{
	VIS vis;
	const std::vector<Run> runs = syncRuns();

	// The header opens with a calibration burst: a 1900 Hz tone, a 10 ms
	// sync pulse, another 1900 Hz tone, then the VIS section. So the
	// calibration pulse is the first sync-band run and the VIS start
	// marker is the very next one.
	size_t cal = runs.size();
	for (size_t i = 0; i < runs.size(); ++i)
	{
		// Skip any sub-millisecond zero-crossing glitches at signal onset.
		if (demod.samp2ms(runs[i].end - runs[i].begin) >= 4.0f)
		{
			cal = i;
			break;
		}
	}
	if (cal + 1 >= runs.size())
		return vis; // header not recognisable; vis.found stays false

	// The VIS section is ten consecutive 30 ms elements, starting at the
	// start marker run:
	//   [start marker] [7 data bits, LSB first] [parity] [stop marker]
	const size_t visStart = runs[cal + 1].begin;
	constexpr float ElementMs = 30.0f;

	// Mean frequency of the central third of VIS element `slot`, sampled
	// clear of the transitions at each element boundary.
	auto element = [&](int slot) -> float
	{
		size_t a = visStart + demod.ms2samp(slot * ElementMs + 10.0f);
		size_t b = visStart + demod.ms2samp(slot * ElementMs + 20.0f);
		return demod.average(a, b);
	};

	// Data bits: 1100 Hz = 1, 1300 Hz = 0, split at SyncPulse (1200 Hz).
	uint8_t code = 0;
	for (int bit = 0; bit < 7; ++bit)
		if (element(1 + bit) < sstv::SyncPulse)
			code |= uint8_t(1u << bit);

	const bool parityBit = element(8) < sstv::SyncPulse;

	vis.found = true;
	vis.code = code;
	// The encoder writes an even-parity bit: it is 1 iff the code has an
	// odd number of set bits.
	vis.parityOK = (std::popcount(code) & 1) == int(parityBit);
	vis.headerEnd = visStart + demod.ms2samp(10 * ElementMs);
	return vis;
}
