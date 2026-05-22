#pragma once
#include <cstddef>
#include <cstdint>

#include "demodulator.hpp"

// Streaming zero-crossing FM demodulator: the inverse of the encoder's
// Synthesizer, producing one instantaneous-frequency estimate per audio
// sample fed to it.
//
// Each completed half-cycle (the span between two zero crossings) implies a
// frequency; that value is held until the next crossing refreshes it. This is
// causal — a sample carries the frequency of the half-cycle just before it —
// so the demodulator needs no look-ahead and keeps only a few values of
// state. The crossing position is stored as an integer sample index plus a
// fraction so precision never decays with stream length.
class ZeroCrossingDemodulator : public Demodulator
{
	uint32_t rate_;
	float prev_ = 0.0f;           // previous input sample
	size_t index_ = 0;            // running count of samples processed
	size_t lastCrossIndex_ = 0;   // integer sample index of the last crossing
	float lastCrossFrac_ = 0.0f;  // sub-sample fraction [0,1] of the last crossing
	bool haveCrossing_ = false;   // a crossing has been seen
	float freq_ = 0.0f;           // frequency of the most recent half-cycle

public:
	explicit ZeroCrossingDemodulator(uint32_t sampleRate);

	float process(float sample) override;
	uint32_t sampleRate() const override { return rate_; }
};
