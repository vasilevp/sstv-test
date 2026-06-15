#include "vis_detector.hpp"

#include <bit>
#include <string>

namespace
{
	constexpr float LeaderHz = 1700.0f; // leader tone is 1900 Hz; well clear of
	                                    // the 1500 Hz porch and <=1300 Hz bits
	constexpr float ElementMs = 30.0f;
	constexpr size_t MaxPending = 16; // bound on simultaneously-open candidates
}

VisDetector::VisDetector(uint32_t sampleRate)
	: rate(sampleRate),
	  leaderWin(samp(100.0f)),
	  ring(leaderWin, 0),
	  trig(sstv::SyncEnterHz, sstv::SyncExitHz)
{
}

sstv::Vis VisDetector::finalize(const Candidate &c) const
{
	sstv::Vis v;
	auto el = [&](int slot) -> float
	{ return c.n[slot] ? float(c.acc[slot] / c.n[slot]) : 0.0f; };

	// Start and stop markers must both be sync tone (~1200 Hz).
	if (el(0) >= sstv::SyncEnterHz || el(9) >= sstv::SyncEnterHz)
		return v;
	// Data bits: 1100 Hz = 1, 1300 Hz = 0, split at SyncPulse (1200 Hz).
	uint8_t code = 0;
	for (int bit = 0; bit < 7; ++bit)
		if (el(1 + bit) < sstv::SyncPulse)
			code |= uint8_t(1u << bit);
	if (std::string(sstv::visModeName(code)) == "unknown")
		return v;

	const bool parityBit = el(8) < sstv::SyncPulse;
	v.found = true;
	v.code = code;
	// The encoder writes an even-parity bit: it is 1 iff the code has an odd
	// number of set bits.
	v.parityOK = (std::popcount(code) & 1) == int(parityBit);
	v.headerEnd = c.start + samp(10 * ElementMs);
	return v;
}

bool VisDetector::update(float freq)
{
	const size_t i = index++;
	const size_t visLen = samp(10 * ElementMs);

	// 1) Finalize any candidate whose full VIS window has now elapsed. The
	//    front candidate has the earliest start, hence the largest age, so it
	//    is always the first to complete. Lock on the first valid one.
	while (!candidates.empty() && i - candidates.front().start >= visLen)
	{
		const sstv::Vis v = finalize(candidates.front());
		candidates.pop_front();
		if (v.found)
		{
			locked = v;
			return true;
		}
	}

	// 2) A sync-band entry begins a new candidate, but only if the preceding
	//    leader window was predominantly leader pitch (>= 60%). leaderCount
	//    still reflects the samples strictly before i — the window slides in
	//    step (4), after this check.
	const bool sub = (freq > 0.0f) && trig.update(freq);
	if (sub && !inSync)
	{
		inSync = true;
		if (leaderCount * 5 >= leaderWin * 3 && candidates.size() < MaxPending)
			candidates.push_back(Candidate{i, {}, {}});
	}
	else if (!sub && inSync)
	{
		inSync = false;
	}

	// 3) Fold this sample into the central third (10..20 ms) of its element
	//    for every open candidate. Survivors all have age < visLen here, so
	//    the slot index stays in range.
	for (Candidate &c : candidates)
	{
		const float relMs = 1000.0f * float(i - c.start) / float(rate);
		const int slot = int(relMs / ElementMs);
		const float offMs = relMs - slot * ElementMs;
		if (slot >= 0 && slot <= 9 && offMs >= 10.0f && offMs < 20.0f)
		{
			c.acc[slot] += freq;
			++c.n[slot];
		}
	}

	// 4) Slide the leader-tone window by one sample.
	const uint8_t bit = freq >= LeaderHz ? 1 : 0;
	leaderCount -= ring[ringPos];
	ring[ringPos] = bit;
	leaderCount += bit;
	if (++ringPos == leaderWin)
		ringPos = 0;

	return false;
}
