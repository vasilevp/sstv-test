#pragma once

// Schmitt trigger / hysteretic comparator: a one-bit latch with two
// thresholds. Once below the "low" threshold the latch enters the "below"
// state and only leaves it when the input rises strictly above the "high"
// threshold (with high > low). The hysteresis gap suppresses per-sample
// chatter when the input dithers around a single threshold under noise —
// the classic noise-immune comparator design.
//
// Modelled on the SchmittTrigger in xdsopl/robot36; we use it on the
// decoder's frequency stream so a noisy sample dipping briefly into the
// sync band doesn't spawn a one-sample false sync pulse.
class SchmittTrigger
{
public:
	SchmittTrigger(float lowThreshold, float highThreshold);

	// Update with a new sample; returns true if the latch is in the
	// "below" state after the update.
	bool update(float value);

	// Force the latch back to "above" (the initial state).
	void reset();

private:
	float low_;
	float high_;
	bool below_ = false;
};
