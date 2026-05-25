#include "simple_moving_average.hpp"

#include <stdexcept>

SimpleMovingAverage::SimpleMovingAverage(std::size_t windowSize)
	: window_(windowSize, 0.0f)
{
	if (windowSize == 0)
		throw std::invalid_argument("SimpleMovingAverage window must be > 0");
}

float SimpleMovingAverage::update(float sample)
{
	// Replace the oldest entry in the ring buffer and slide the running sum.
	sum_ -= window_[pos_];
	window_[pos_] = sample;
	sum_ += sample;
	if (++pos_ >= window_.size())
		pos_ = 0;
	return sum_ / static_cast<float>(window_.size());
}

void SimpleMovingAverage::reset()
{
	for (float &v : window_)
		v = 0.0f;
	pos_ = 0;
	sum_ = 0.0f;
}
