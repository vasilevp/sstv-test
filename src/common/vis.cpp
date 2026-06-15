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

	// Robot
	case RobotColor12:
		return "Robot Color 12";
	case RobotColor24:
		return "Robot Color 24";
	case RobotColor36:
		return "Robot Color 36";
	case RobotColor72:
		return "Robot Color 72";
	case RobotBW8_R:
	case RobotBW8_G:
	case RobotBW8_B:
		return "Robot B&W 8";
	case RobotBW12_R:
	case RobotBW12_G:
	case RobotBW12_B:
		return "Robot B&W 12";
	case RobotBW24_R:
	case RobotBW24_G:
	case RobotBW24_B:
		return "Robot B&W 24";
	case RobotBW36_R:
	case RobotBW36_G:
	case RobotBW36_B:
		return "Robot B&W 36";

	// Martin
	case MartinM1:
		return "Martin M1";
	case MartinM2:
		return "Martin M2";
	case MartinM3:
		return "Martin M3";
	case MartinM4:
		return "Martin M4";
	case MartinHQ1:
		return "Martin HQ1";
	case MartinHQ2:
		return "Martin HQ2";

	// Scottie
	case ScottieS1:
		return "Scottie S1";
	case ScottieS2:
		return "Scottie S2";
	case ScottieS3:
		return "Scottie S3";
	case ScottieS4:
		return "Scottie S4";
	case ScottieDX:
		return "Scottie DX";
	case ScottieDX2:
		return "Scottie DX2";

	// PD
	case PD50:
		return "PD 50";
	case PD90:
		return "PD 90";
	case PD120:
		return "PD 120";
	case PD160:
		return "PD 160";
	case PD180:
		return "PD 180";
	case PD240:
		return "PD 240";
	case PD290:
		return "PD 290";

	// Wraase
	case WraaseSC1_24:
		return "Wraase SC1 24";
	case WraaseSC1_48:
		return "Wraase SC1 48";
	case WraaseSC1_48Q:
		return "Wraase SC1 48Q";
	case WraaseSC1_96:
		return "Wraase SC1 96";
	case WraaseSC2_30:
		return "Wraase SC2 30";
	case WraaseSC2_60:
		return "Wraase SC2 60";
	case WraaseSC2_120:
		return "Wraase SC2 120";
	case WraaseSC2_180:
		return "Wraase SC2 180";

	// Color FAX
	case ColorFAX8_R:
	case ColorFAX8_G:
	case ColorFAX8_B:
		return "Color FAX 8";
	case ColorFAX16_R:
	case ColorFAX16_G:
	case ColorFAX16_B:
		return "Color FAX 16";
	case ColorFAX24_R:
	case ColorFAX24_G:
	case ColorFAX24_B:
		return "Color FAX 24";
	case ColorFAX32_R:
	case ColorFAX32_G:
	case ColorFAX32_B:
		return "Color FAX 32";

	// AVT
	case AVT24:
	case AVT24_Narrow:
	case AVT24_QRM:
	case AVT24_NarrowQRM:
		return "AVT 24";
	case AVT90:
	case AVT90_Narrow:
	case AVT90_QRM:
	case AVT90_NarrowQRM:
		return "AVT 90";
	case AVT94:
	case AVT94_Narrow:
	case AVT94_QRM:
	case AVT94_NarrowQRM:
		return "AVT 94";
	case AVT188_Narrow:
	case AVT188_QRM:
	case AVT188_NarrowQRM:
		return "AVT 188";
	case AVT125BW_Narrow:
	case AVT125BW_QRM:
	case AVT125BW_NarrowQRM:
		return "AVT 125 BW";

	// Pasokon
	case PasokonP3:
		return "Pasokon P3";
	case PasokonP5:
		return "Pasokon P5";
	case PasokonP7:
		return "Pasokon P7";

	// MSCAN
	case MSCANTV1:
		return "MSCAN TV-1";
	case MSCANTV2:
		return "MSCAN TV-2";

	// Single-mode systems
	case ProskanJ120:
		return "Proskan J120";
	case Vester:
		return "Vester";
	case FAX480:
		return "FAX480";
	case FastFM:
		return "FAST FM";
	case SP17BW:
		return "SP-17 BW";
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
	case AVT24:
	case AVT24_Narrow:
	case AVT24_QRM:
	case AVT24_NarrowQRM:
	case ColorFAX8_R:
	case ColorFAX8_G:
	case ColorFAX8_B:
	case ColorFAX24_R:
	case ColorFAX24_G:
	case ColorFAX24_B:
	case WraaseSC1_24:
	case WraaseSC1_48Q:
	case SP17BW:
		return 128;
	case AVT90:
	case AVT90_Narrow:
	case AVT90_QRM:
	case AVT90_NarrowQRM:
	case ColorFAX16_R:
	case ColorFAX16_G:
	case ColorFAX16_B:
	case ColorFAX32_R:
	case ColorFAX32_G:
	case ColorFAX32_B:
	case WraaseSC1_48:
	case WraaseSC1_96:
		return 256;
	case FAX480:
	case Vester:
	case PD160:
		return 512;
	// PD family. The high-res variants are the reason this table exists:
	// without it a PD120 frame decodes at the 320 default and comes out
	// stretched 2:1 vertically (the row count is fixed by the transmission).
	case PD120:
	case PD180:
	case PD240:
	case PasokonP3:
	case PasokonP5:
	case PasokonP7:
		return 640;
	case PD290:
		return 800;
	// Everything else — Robot 36/72, Martin, Scottie, the AVT 94/188/125 and
	// Wraase SC2 families, MSCAN, Proskan, FAST FM, PD 50/90 — is the classic
	// 320-wide grid, as is any unrecognised code.
	default:
		return 320;
	}
}

bool sstv::isKnownMode(VisCode code)
{
	return std::string_view(modeName(code)) != "unknown";
}
