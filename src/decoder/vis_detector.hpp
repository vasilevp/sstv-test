#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "schmitt_trigger.hpp"
#include "common/vis.hpp"

// SSTV tone frequencies (Hz) — the decoding-side mirror of the encoder's
// Synthesizer::Frequency enum.
namespace sstv
{
	constexpr float Black = 1500.0f;     // pixel luma 0
	constexpr float White = 2300.0f;     // pixel luma 255
	constexpr float SyncPulse = 1200.0f; // scanline / VIS sync tone

	constexpr float VISOne = 1100.0f;  // a VIS data bit valued 1
	constexpr float VISZero = 1300.0f; // a VIS data bit valued 0

	// Schmitt-trigger thresholds for sync detection. Enter the "sync"
	// (below) state when freq drops below SyncEnterHz; exit only when freq
	// rises strictly above the higher SyncExitHz. The 75 Hz hysteresis gap
	// suppresses per-sample threshold chatter that would otherwise spawn
	// spurious short sync runs from noise. Values follow the xdsopl/robot36
	// design:
	//   SyncExitHz  = midpoint(SyncPulse, Porch)             = (1200+1500)/2 = 1350
	//   SyncEnterHz = midpoint(SyncPulse, SyncExitHz)        = (1200+1350)/2 = 1275
	constexpr float SyncEnterHz = 1275.0f;
	constexpr float SyncExitHz = 1350.0f;

}

// Streaming detector for the calibration / VIS header.
//
// Fed one demodulated frequency sample at a time and keeps all the state it
// needs between calls, so detection costs O(1) per sample — nothing is ever
// rescanned. This is what makes the decoder real-time capable: a live receiver
// can run it sample-by-sample as audio arrives.
//
// The VIS section is ten consecutive 30 ms elements:
//   [start marker] [7 data bits, LSB first] [parity] [stop marker]
// The start/stop markers are 1200 Hz sync tone; data/parity bits are 1100 Hz
// (= 1) or 1300 Hz (= 0). A genuine start marker is immediately preceded by
// the calibration header's ~300 ms 1900 Hz leader tone — the feature that
// tells the real header apart from the seconds of voice and static that
// precede it in off-air recordings. So every time the signal drops into the
// sync band after a sustained leader tone, that position is taken as a
// candidate start marker; the following 300 ms are accumulated per element and
// validated (markers, mode code, parity). The detector locks on the first
// candidate whose markers and mode code are self-consistent.
class VisDetector
{
public:
	explicit VisDetector(uint32_t sampleRate);

	// Process one demodulated frequency sample (Hz). Returns true on the
	// sample that completes a valid VIS section, after which result() holds
	// the decoded code and headerEnd is the index of that very sample (the
	// first sample past the stop marker).
	bool update(float freq);

	const sstv::Vis &result() const { return locked; }

private:
	// One in-progress candidate VIS section: running mean of each element's
	// central third, accumulated as samples stream past.
	struct Candidate
	{
		size_t start = 0;
		double acc[10] = {};
		uint32_t n[10] = {};
	};

	size_t samp(float ms) const { return size_t(rate * ms / 1000.0f); }
	sstv::Vis finalize(const Candidate &c) const;

	uint32_t rate;
	size_t leaderWin;            // sliding leader-tone window length, samples
	std::vector<uint8_t> ring;   // per-sample "is leader pitch" over leaderWin
	size_t ringPos = 0;
	size_t leaderCount = 0;      // leader-pitch samples currently in the window
	SchmittTrigger trig;         // sync-band entry detector
	bool inSync = false;
	size_t index = 0;            // running sample count
	std::deque<Candidate> candidates;
	sstv::Vis locked;
};
