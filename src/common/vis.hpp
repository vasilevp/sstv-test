#pragma once
#include <cstddef>
#include <cstdint>

// SSTV mode taxonomy: the VIS (Vertical Interval Signalling) code the
// transmitter sends in the header to declare its mode, plus the per-mode
// metadata the decoder needs (human-readable name, native pixel width).
//
// All codes, names and dimensions follow Bruchanov, "Image Communication on
// Short Waves" (sstv-handbook.com), chapter 5 "List of SSTV modes".
namespace sstv
{
	// The 7-bit VIS mode code, catalogued from the SSTV Handbook (Bruchanov,
	// ch. 5). The underlying type holds any 7-bit value that arrives off the
	// air, so an unrecognised code is a representable VisCode for which
	// modeName() reports "unknown" and isKnownMode() returns false. Only a
	// handful of these have a decoder built for them; the rest are named so the
	// detector can recognise the header, after which the decoder falls back.
	//
	// Naming conventions for the multi-code modes:
	//   * The B&W families (Robot B&W, Color FAX) send one code per R/G/B
	//     filter component — _R / _G / _B all denote the same mode.
	//   * The AVT modes send four codes: a base (Normal) plus _Narrow, _QRM and
	//     _NarrowQRM transmission variants.
	//
	// Two codes are shared (the handbook flags these as accidental): 76 is both
	// Scottie DX and AVT 188's Normal code, and 80 is both Scottie DX2 and AVT
	// 125 BW's Normal code. The Scottie name wins each; the AVT modes keep their
	// three remaining variant codes.
	//
	// Excluded because they don't fit a single byte: the MP/MR modes (16-bit
	// VIS) and the MMSSTV "-N" modes (separate N-VIS code space). Martin HQ3/HQ4
	// are omitted too — the handbook lists their codes as unknown.
	enum class VisCode : uint8_t
	{
		RobotColor12 = 0,
		RobotBW8_R = 1,
		RobotBW8_G = 2,
		RobotBW8_B = 3,
		RobotColor24 = 4,
		RobotBW12_R = 5,
		RobotBW12_G = 6,
		RobotBW12_B = 7,
		RobotColor36 = 8,
		RobotBW24_R = 9,
		RobotBW24_G = 10,
		RobotBW24_B = 11,
		RobotColor72 = 12,
		RobotBW36_R = 13,
		RobotBW36_G = 14,
		RobotBW36_B = 15,
		WraaseSC1_24 = 16,
		ColorFAX8_R = 17,
		ColorFAX8_G = 18,
		ColorFAX8_B = 19,
		WraaseSC1_48 = 20,
		ColorFAX16_R = 21,
		ColorFAX16_G = 22,
		ColorFAX16_B = 23,
		WraaseSC1_48Q = 24,
		ColorFAX24_R = 25,
		ColorFAX24_G = 26,
		ColorFAX24_B = 27,
		WraaseSC1_96 = 28,
		ColorFAX32_R = 29,
		ColorFAX32_G = 30,
		ColorFAX32_B = 31,
		MartinM4 = 32,
		MartinM3 = 36,
		MartinM2 = 40,
		MartinHQ1 = 41,
		MartinHQ2 = 42,
		MartinM1 = 44,
		ScottieS4 = 48,
		WraaseSC2_30 = 51,
		ScottieS3 = 52,
		WraaseSC2_180 = 55,
		ScottieS2 = 56,
		WraaseSC2_60 = 59,
		ScottieS1 = 60,
		WraaseSC2_120 = 63,
		AVT24 = 64,
		AVT24_Narrow = 65,
		AVT24_QRM = 66,
		AVT24_NarrowQRM = 67,
		AVT90 = 68,
		AVT90_Narrow = 69,
		AVT90_QRM = 70,
		AVT90_NarrowQRM = 71,
		AVT94 = 72,
		AVT94_Narrow = 73,
		AVT94_QRM = 74,
		AVT94_NarrowQRM = 75,
		ScottieDX = 76, // shared with AVT 188 Normal
		AVT188_Narrow = 77,
		AVT188_QRM = 78,
		AVT188_NarrowQRM = 79,
		ScottieDX2 = 80, // shared with AVT 125 BW Normal
		AVT125BW_Narrow = 81,
		AVT125BW_QRM = 82,
		AVT125BW_NarrowQRM = 83,
		FAX480 = 85,
		Vester = 86,
		FastFM = 90,
		PD50 = 93,
		PD290 = 94,
		PD120 = 95,
		PD180 = 96,
		PD240 = 97,
		PD160 = 98,
		PD90 = 99,
		ProskanJ120 = 100,
		MSCANTV1 = 104,
		MSCANTV2 = 105,
		PasokonP3 = 113,
		PasokonP5 = 114,
		PasokonP7 = 115,
		SP17BW = 125,
	};

	// Human-readable mode name for a VIS code, or "unknown".
	const char *modeName(VisCode code);

	// Native scanline width (luma pixels per line) — the "Columns" field of the
	// handbook's mode table, i.e. the pixel grid the transmitter sampled its
	// image into. The decoder resamples each line to this many columns so the
	// output keeps the mode's intended aspect ratio (320 for most modes; the
	// high-res PD modes the ISS uses are wider). Unknown codes fall back to 320.
	uint32_t modeWidth(VisCode code);

	// True if `code` names a mode the handbook table recognises.
	bool isKnownMode(VisCode code);

	// Decoded header VIS result: the outcome of probing a recording for its
	// calibration + VIS section.
	struct Vis
	{
		bool found = false;            // the header VIS section was located
		VisCode code = VisCode{0};     // decoded 7-bit mode code (valid if found)
		bool parityOK = false;         // decoded parity bit matched the code
		size_t headerEnd = 0;          // sample index just past the VIS stop marker

		// Convenience accessors for the code's metadata.
		const char *name() const { return modeName(code); }
		uint32_t width() const { return modeWidth(code); }
	};
}
