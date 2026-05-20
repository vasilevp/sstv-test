#include "demodulator.hpp"

#include <cmath>

#include "wav.hpp"

Demodulator::Demodulator(const std::string &wavPath)
{
	WAVReader wav(wavPath);
	sample_rate_ = wav.sampleRate();
	const std::vector<float> &s = wav.samples();

	freq_.assign(s.size(), 0.0f);
	if (s.size() < 2)
		return;

	// Remove any DC bias so the sign of each sample is meaningful.
	double mean = 0.0;
	for (float v : s)
		mean += v;
	mean /= double(s.size());

	// Locate every zero crossing, linearly interpolating the fractional
	// sample position so the half-period measurement is sub-sample accurate.
	std::vector<double> crossings;
	double prev = s[0] - mean;
	for (size_t i = 1; i < s.size(); ++i)
	{
		double cur = s[i] - mean;
		if ((prev < 0.0 && cur >= 0.0) || (prev > 0.0 && cur <= 0.0))
		{
			double frac = prev / (prev - cur); // in [0, 1]
			crossings.push_back(double(i - 1) + frac);
		}
		prev = cur;
	}

	// The gap between two adjacent crossings is one half-period; the
	// frequency it implies is held across every sample that gap spans.
	for (size_t k = 0; k + 1 < crossings.size(); ++k)
	{
		double half = crossings[k + 1] - crossings[k];
		if (half <= 0.0)
			continue;
		float f = float(0.5 * double(sample_rate_) / half);

		size_t begin = size_t(std::lround(crossings[k]));
		size_t end = size_t(std::lround(crossings[k + 1]));
		for (size_t i = begin; i < end && i < freq_.size(); ++i)
			freq_[i] = f;
	}
}
