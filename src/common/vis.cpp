#include "common/vis.hpp"

#include <string_view>
#include <utility>

// The name table is keyed on the raw 7-bit value rather than the VisCode
// enumerators: it names every mode the handbook lists, including ones with no
// enumerator (and no decoder) such as Robot Color 24 and the Robot B&W
// 12/24/36 triplets. isKnownMode() leans on this to validate a freshly decoded
// VIS code before the detector commits to it.
const char *sstv::modeName(VisCode code)
{
	switch (std::to_underlying(code))
	{
	case 0:
		return "Robot Color 12";
	case 1:
	case 2:
	case 3:
		return "Robot B&W 8";
	case 4:
		return "Robot Color 24";
	case 5:
	case 6:
	case 7:
		return "Robot B&W 12";
	case 8:
		return "Robot Color 36";
	case 9:
	case 10:
	case 11:
		return "Robot B&W 24";
	case 12:
		return "Robot Color 72";
	case 13:
	case 14:
	case 15:
		return "Robot B&W 36";
	case 32:
		return "Martin M4";
	case 36:
		return "Martin M3";
	case 40:
		return "Martin M2";
	case 44:
		return "Martin M1";
	case 48:
		return "Scottie S4";
	case 52:
		return "Scottie S3";
	case 56:
		return "Scottie S2";
	case 60:
		return "Scottie S1";
	case 76:
		return "Scottie DX";
	case 93:
		return "PD 50";
	case 94:
		return "PD 290";
	case 95:
		return "PD 120";
	case 96:
		return "PD 180";
	case 97:
		return "PD 240";
	case 98:
		return "PD 160";
	case 99:
		return "PD 90";
	default:
		return "unknown";
	}
}

uint32_t sstv::modeWidth(VisCode code)
{
	switch (std::to_underlying(code))
	{
	// Robot B&W 8 (one VIS per R/G/B filter component). The handbook's row is
	// internally transposed (it prints Lines=160/Columns=120, but 900 lpm over
	// 8 s is 120 lines); the consistent, canonical frame is 160x120.
	case 1:
	case 2:
	case 3:
		return 160;
	// PD family. The high-res variants are the reason this table exists:
	// without it a PD120 frame decodes at the 320 default and comes out
	// stretched 2:1 vertically (the row count is fixed by the transmission).
	case 94: // PD 290
		return 800;
	case 98: // PD 160
		return 512;
	case 95: // PD 120
	case 96: // PD 180
	case 97: // PD 240
		return 640;
	// Everything else this decoder builds — Robot 36/72, Martin M1..M4,
	// Scottie S1..S4/DX, PD 50/90 — is the classic 320-wide grid.
	default:
		return 320;
	}
}

bool sstv::isKnownMode(VisCode code)
{
	return std::string_view(modeName(code)) != "unknown";
}
