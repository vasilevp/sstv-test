#include "schmitt_trigger.hpp"

SchmittTrigger::SchmittTrigger(float lowThreshold, float highThreshold)
	: low_(lowThreshold), high_(highThreshold)
{
}

bool SchmittTrigger::update(float value)
{
	if (below_)
	{
		if (value > high_)
			below_ = false;
	}
	else
	{
		if (value < low_)
			below_ = true;
	}
	return below_;
}

void SchmittTrigger::reset()
{
	below_ = false;
}
