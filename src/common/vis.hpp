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
	// The 7-bit VIS mode code. Every mode the handbook lists has an enumerator;
	// the underlying type still holds any 7-bit value that arrives off the air,
	// so an unrecognised code is a representable VisCode for which modeName()
	// reports "unknown" and isKnownMode() returns false.
	//
	// The Robot B&W modes occupy three consecutive codes each (one per R/G/B
	// filter component); all three denote the same mode, just a different
	// colour-channel selection. Some recognised modes (Robot Color 24, Robot
	// B&W 12/24/36) have no decoder built for them — they are named, but the
	// decoder falls back when one turns up.
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
		MartinM4 = 32,
		MartinM3 = 36,
		MartinM2 = 40,
		MartinM1 = 44,
		ScottieS4 = 48,
		ScottieS3 = 52,
		ScottieS2 = 56,
		ScottieS1 = 60,
		ScottieDX = 76,
		PD50 = 93,
		PD290 = 94,
		PD120 = 95,
		PD180 = 96,
		PD240 = 97,
		PD160 = 98,
		PD90 = 99,
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
