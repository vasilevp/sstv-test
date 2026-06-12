// Correctness test for the streaming demodulators.
//
// Feeds each Demodulator a known pure sine and checks that the reported
// instantaneous frequency converges to the input frequency, across the SSTV
// tone range (1100..2300 Hz). Also measures step response and long-stream
// precision so we can spot any drift the integer/fraction split was meant to
// remove.

#include "demodulator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numbers>
#include <print>
#include <vector>

namespace
{
	constexpr double Pi = std::numbers::pi_v<double>;
	constexpr uint32_t Rate = 8000;

	// Generate one normalised audio sample at frequency `freq` Hz, taking the
	// phase from the integer sample index `i` so float precision never bites
	// — useful when generating tens of millions of samples for the long-run
	// precision test.
	float sineSample(double freq, size_t i)
	{
		const double t = double(i) / double(Rate);
		return float(std::sin(2.0 * Pi * freq * t));
	}

	struct Stats
	{
		float mean = 0.0f;
		float stddev = 0.0f;
		float minimum = 0.0f;
		float maximum = 0.0f;
	};

	Stats analyse(const std::vector<float> &xs)
	{
		Stats s;
		if (xs.empty())
			return s;
		s.minimum = xs.front();
		s.maximum = xs.front();
		double sum = 0.0;
		for (float x : xs)
		{
			sum += x;
			if (x < s.minimum)
				s.minimum = x;
			if (x > s.maximum)
				s.maximum = x;
		}
		s.mean = float(sum / double(xs.size()));
		double var = 0.0;
		for (float x : xs)
		{
			const double d = double(x) - double(s.mean);
			var += d * d;
		}
		s.stddev = float(std::sqrt(var / double(xs.size())));
		return s;
	}

	const char *demodName(DemodKind k) { return demodKindName(k); }
}

int main()
{
	std::println("Demodulator correctness test (sample rate {} Hz)\n", Rate);

	// --- Steady-state accuracy ------------------------------------------
	// 1 s tone at each target frequency, discard the first 50 ms so the
	// LPF in the IQ demodulator has time to settle.
	std::println("== Steady-state (1 s tone, first 50 ms discarded) ==");
	std::println("{:<6}  {:<13}  {:>10}  {:>10}  {:>10}  {:>10}",
	             "freq", "demod", "mean", "error", "stddev", "range");

	const std::vector<float> testFreqs = {1100, 1200, 1300, 1500, 1700, 1900, 2100, 2300};
	const size_t analyseFrom = Rate / 20; // 50 ms

	for (float f : testFreqs)
	{
		for (auto kind : {DemodKind::ZeroCrossing, DemodKind::Quadrature})
		{
			auto d = makeDemodulator(kind, Rate);
			std::vector<float> reported;
			reported.reserve(Rate - analyseFrom);
			for (size_t i = 0; i < Rate; ++i)
			{
				const float v = d->process(sineSample(f, i));
				if (i >= analyseFrom)
					reported.push_back(v);
			}
			const Stats s = analyse(reported);
			std::println("{:6.0f}  {:<13}  {:10.4f}  {:+10.4f}  {:10.4f}  {:10.4f}",
			             f, demodName(kind),
			             s.mean, s.mean - f, s.stddev, s.maximum - s.minimum);
		}
	}

	// --- Step response --------------------------------------------------
	// Switch the input from 1900 Hz to 1200 Hz at t=0 and time how long
	// each demodulator takes to land within ±1 % of the new value.
	std::println("\n== Step response (1900 Hz -> 1200 Hz, settling to ±1%) ==");
	std::println("{:<13}  {:>12}  {:>12}", "demod", "first <1%", "settled <1%");
	for (auto kind : {DemodKind::ZeroCrossing, DemodKind::Quadrature})
	{
		auto d = makeDemodulator(kind, Rate);
		// Settle on 1900 Hz first.
		for (size_t i = 0; i < Rate / 5; ++i)
			(void)d->process(sineSample(1900.0, i));

		const float target = 1200.0f;
		const float tol = 12.0f; // 1% of 1200
		long firstWithin = -1;
		long settled = -1;
		long consecutive = 0;
		for (size_t i = 0; i < Rate / 5; ++i) // 200 ms
		{
			const float v = d->process(sineSample(target, i));
			const bool ok = std::abs(v - target) < tol;
			if (firstWithin < 0 && ok)
				firstWithin = long(i);
			if (ok)
				++consecutive;
			else
				consecutive = 0;
			if (settled < 0 && consecutive >= long(Rate / 100)) // sustained 10 ms
				settled = long(i) - long(Rate / 100) + 1;
		}
		auto ms = [](long n) { return n < 0 ? -1.0 : 1000.0 * double(n) / double(Rate); };
		std::println("{:<13}  {:9.2f} ms  {:9.2f} ms",
		             demodName(kind), ms(firstWithin), ms(settled));
	}

	// --- Long-stream precision ------------------------------------------
	// 30 minutes of 1900 Hz — well past float's 24-bit integer-exact
	// horizon (~35 min at 8 kHz). Tests that neither demodulator has
	// developed precision drift over the run.
	std::println("\n== Long-stream precision (30 min @ 1900 Hz, last 100 ms) ==");
	std::println("{:<13}  {:>10}  {:>10}", "demod", "mean", "error");
	const size_t longN = size_t(Rate) * 1800;
	const size_t longSkip = longN - Rate / 10;
	for (auto kind : {DemodKind::ZeroCrossing, DemodKind::Quadrature})
	{
		auto d = makeDemodulator(kind, Rate);
		double sum = 0.0;
		size_t count = 0;
		for (size_t i = 0; i < longN; ++i)
		{
			const float v = d->process(sineSample(1900.0, i));
			if (i >= longSkip)
			{
				sum += v;
				++count;
			}
		}
		const double mean = sum / double(count);
		std::println("{:<13}  {:10.4f}  {:+10.4f}",
		             demodName(kind), mean, mean - 1900.0);
	}

	return 0;
}
