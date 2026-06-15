#include "common/vis.hpp"

#include <string_view>

// Names every mode the handbook lists, including ones with no decoder (Robot
// Color 24, the Robot B&W 12/24/36 triplets). isKnownMode() leans on this to
// validate a freshly decoded VIS code before the detector commits to it.
const char *sstv::modeName(VisCode code)
{
	switch (code)
	{
		using enum VisCode;
	case RobotColor12:
		return "Robot Color 12";
	case RobotBW8_R:
	case RobotBW8_G:
	case RobotBW8_B:
		return "Robot B&W 8";
	case RobotColor24:
		return "Robot Color 24";
	case RobotBW12_R:
	case RobotBW12_G:
	case RobotBW12_B:
		return "Robot B&W 12";
	case RobotColor36:
		return "Robot Color 36";
	case RobotBW24_R:
	case RobotBW24_G:
	case RobotBW24_B:
		return "Robot B&W 24";
	case RobotColor72:
		return "Robot Color 72";
	case RobotBW36_R:
	case RobotBW36_G:
	case RobotBW36_B:
		return "Robot B&W 36";
	case MartinM4:
		return "Martin M4";
	case MartinM3:
		return "Martin M3";
	case MartinM2:
		return "Martin M2";
	case MartinM1:
		return "Martin M1";
	case ScottieS4:
		return "Scottie S4";
	case ScottieS3:
		return "Scottie S3";
	case ScottieS2:
		return "Scottie S2";
	case ScottieS1:
		return "Scottie S1";
	case ScottieDX:
		return "Scottie DX";
	case PD50:
		return "PD 50";
	case PD290:
		return "PD 290";
	case PD120:
		return "PD 120";
	case PD180:
		return "PD 180";
	case PD240:
		return "PD 240";
	case PD160:
		return "PD 160";
	case PD90:
		return "PD 90";
	}
	return "unknown";
}

uint32_t sstv::modeWidth(VisCode code)
{
	switch (code)
	{
		using enum VisCode;
	// Robot B&W 8 (one VIS per R/G/B filter component). The handbook's row is
	// internally transposed (it prints Lines=160/Columns=120, but 900 lpm over
	// 8 s is 120 lines); the consistent, canonical frame is 160x120.
	case RobotBW8_R:
	case RobotBW8_G:
	case RobotBW8_B:
		return 160;
	// PD family. The high-res variants are the reason this table exists:
	// without it a PD120 frame decodes at the 320 default and comes out
	// stretched 2:1 vertically (the row count is fixed by the transmission).
	case PD290:
		return 800;
	case PD160:
		return 512;
	case PD120:
	case PD180:
	case PD240:
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
