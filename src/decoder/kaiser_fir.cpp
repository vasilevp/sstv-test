#include "kaiser_fir.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
	constexpr float Pi = std::numbers::pi_v<float>;

	// Modified Bessel function of the first kind, order 0 — needed by
	// the Kaiser window. Power series; converges fast for the β values
	// the Kaiser design produces (typically 4..7).
	float besselI0(float x)
	{
		const float halfX = 0.5f * x;
		float term = 1.0f;
		float sum = 1.0f;
		for (int k = 1; k < 64; ++k)
		{
			const float r = halfX / float(k);
			term *= r * r;
			sum += term;
			if (term < 1e-9f * sum)
				break;
		}
		return sum;
	}

	// Kaiser's β formula (Oppenheim & Schafer, eq. 10.13). Maps a desired
	// stopband attenuation in dB to the window parameter.
	float kaiserBeta(float attenDb)
	{
		if (attenDb > 50.0f)
			return 0.1102f * (attenDb - 8.7f);
		if (attenDb >= 21.0f)
			return 0.5842f * std::pow(attenDb - 21.0f, 0.4f)
			     + 0.07886f * (attenDb - 21.0f);
		return 0.0f;
	}
}

KaiserFir::KaiserFir() = default;

void KaiserFir::setLowpass(float cutoffHz, float transitionHz, float sampleRate,
                           float stopbandAttenDb)
{
	const float beta = kaiserBeta(stopbandAttenDb);
	// Kaiser/Schafer tap-count estimate. Δω is the transition width in
	// radians per sample.
	const float deltaOmega = 2.0f * Pi * transitionHz / sampleRate;
	int n = int(std::ceil((stopbandAttenDb - 8.0f) / (2.285f * deltaOmega)));
	if (n < 1)
		n = 1;
	// Force an odd length so the impulse response is symmetric around
	// an integer index — keeps the linear-phase delay an exact integer
	// (N-1)/2 samples.
	if ((n & 1) == 0)
		++n;
	const int N = n;
	const int M = (N - 1) / 2;

	// Ideal LPF impulse response: 2fc · sinc(2fc·k). The cutoff is placed
	// at the midpoint of the transition band so the -6 dB point sits where
	// the user asked for it (matches typical Kaiser-design convention).
	const float fc = (cutoffHz + 0.5f * transitionHz) / sampleRate;
	const float twoPiFc = 2.0f * Pi * fc;

	taps_.assign(N, 0.0f);
	const float i0Beta = besselI0(beta);
	for (int k = 0; k < N; ++k)
	{
		const int j = k - M;  // centred index in [-M, M].
		const float sinc = j == 0
			? 2.0f * fc
			: std::sin(twoPiFc * float(j)) / (Pi * float(j));
		const float r = M > 0 ? float(j) / float(M) : 0.0f;
		const float window =
			besselI0(beta * std::sqrt(std::max(0.0f, 1.0f - r * r))) / i0Beta;
		taps_[k] = sinc * window;
	}

	// Normalise for unity DC gain — the truncated/windowed sinc would
	// otherwise drift a fraction of a dB off.
	float sum = 0.0f;
	for (float t : taps_)
		sum += t;
	if (sum > 0.0f)
		for (float &t : taps_)
			t /= sum;

	ring_.assign(N, 0.0f);
	pos_ = 0;
}

float KaiserFir::process(float input)
{
	if (taps_.empty())
		return input;
	const std::size_t N = ring_.size();
	ring_[pos_] = input;

	// Convolve newest-to-oldest. taps_[0] is the oldest end of the impulse
	// response — pair it with the oldest x[] in the ring; taps_[N-1] is the
	// newest end — pair it with the freshly written sample.
	float acc = 0.0f;
	std::size_t r = pos_;
	for (std::size_t k = N; k-- > 0;)
	{
		acc += taps_[k] * ring_[r];
		r = r == 0 ? N - 1 : r - 1;
	}

	pos_ = pos_ + 1 == N ? 0 : pos_ + 1;
	return acc;
}

void KaiserFir::reset()
{
	std::fill(ring_.begin(), ring_.end(), 0.0f);
	pos_ = 0;
}
