// Correctness test for the encoder (the modulator) against the SSTV
// Handbook (Bruchanov, "Image Communication on Short Waves", ch. 5).
//
// Reads each WAV the encoder emits, then checks two things end-to-end:
//
//   1. Mean scanline period — does the emitted line cadence match the
//      handbook's lpm figure? Measured as the average end-to-end interval
//      between consecutive scanline sync pulses, well past the header.
//      Run via the zero-crossing demodulator: it has comfortable
//      threshold-detection headroom in the sync band (per the demod
//      correctness test) and so cleanly delimits the sync pulses.
//
//   2. Calibration Grey frequency — the 300 ms header tone before the
//      cal pulse should be 1900 Hz exactly. Run via the quadrature
//      demodulator: it has zero mean bias, so any deviation it reports
//      is the modulator's, not the demod's.

#include "decoder.hpp"
#include "demodulator.hpp"
#include "wav.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
	struct ModeSpec
	{
		std::string wav;
		std::string name;
		float lpm;          // lines per minute, per the handbook
		int linesPerSync;   // 1 normally, 2 for PD (one sync per scanline pair)
	};

	const std::vector<ModeSpec> modes = {
		{"outputs/robot8.wav",    "Robot B&W 8",    900.000f, 1},
		{"outputs/robot36.wav",   "Robot Color 36", 400.000f, 1},
		{"outputs/robot72.wav",   "Robot Color 72", 200.000f, 1},
		{"outputs/martin1.wav",   "Martin M1",      134.395f, 1},
		{"outputs/martin2.wav",   "Martin M2",      264.553f, 1},
		{"outputs/martin3.wav",   "Martin M3",      134.395f, 1},
		{"outputs/martin4.wav",   "Martin M4",      264.553f, 1},
		{"outputs/scottie1.wav",  "Scottie S1",     140.115f, 1},
		{"outputs/scottie2.wav",  "Scottie S2",     216.067f, 1},
		{"outputs/scottie3.wav",  "Scottie S3",     140.115f, 1},
		{"outputs/scottie4.wav",  "Scottie S4",     216.067f, 1},
		{"outputs/scottieDX.wav", "Scottie DX",      57.127f, 1},
		{"outputs/pd50.wav",      "PD 50",          309.151f, 2},
		{"outputs/pd90.wav",      "PD 90",          170.687f, 2},
		{"outputs/pd120.wav",     "PD 120",         235.997f, 2},
		{"outputs/pd160.wav",     "PD 160",         149.177f, 2},
		{"outputs/pd180.wav",     "PD 180",         159.101f, 2},
		{"outputs/pd240.wav",     "PD 240",         120.000f, 2},
		{"outputs/pd290.wav",     "PD 290",         128.030f, 2},
	};

	std::vector<float> demodulate(const std::vector<float> &samples,
	                              uint32_t rate, DemodKind kind)
	{
		auto d = makeDemodulator(kind, rate);
		std::vector<float> freq;
		freq.reserve(samples.size());
		for (float s : samples)
			freq.push_back(d->process(s));
		return freq;
	}

	struct Run
	{
		size_t begin, end;
	};

	std::vector<Run> findSyncRuns(const std::vector<float> &freq)
	{
		std::vector<Run> runs;
		for (size_t i = 0; i < freq.size();)
		{
			if (freq[i] > 0.0f && freq[i] < 1280.0f)
			{
				size_t b = i;
				while (i < freq.size() && freq[i] > 0.0f && freq[i] < 1280.0f)
					++i;
				runs.push_back({b, i});
			}
			else
			{
				++i;
			}
		}
		return runs;
	}

	float meanFreq(const std::vector<float> &freq, size_t a, size_t b)
	{
		if (a >= b || b > freq.size())
			return 0.0f;
		double s = 0.0;
		for (size_t i = a; i < b; ++i)
			s += freq[i];
		return float(s / double(b - a));
	}
}

int main()
{
	std::printf("Modulator correctness vs SSTV Handbook "
	            "(Bruchanov ch. 5, sstv-handbook.com)\n\n");
	std::printf("%-17s  %12s  %12s  %8s   %12s\n",
	            "Mode", "expect (ms)", "actual (ms)", "err",
	            "Grey freq (Hz)");
	std::printf("%-17s  %12s  %12s  %8s   %12s\n",
	            "-----------------", "------------", "------------",
	            "--------", "--------------");

	int passed = 0;
	int failed = 0;
	bool anyFail = false;

	for (const ModeSpec &m : modes)
	{
		WAVReader wav(m.wav);
		const uint32_t rate = wav.sampleRate();
		// Drain the streaming source into a vector — this offline test
		// wants random-access slicing into the freq buffer, so streaming
		// doesn't buy anything here.
		std::vector<float> samples;
		std::vector<float> block(4096);
		while (true)
		{
			const std::size_t n = wav.read(block);
			if (n == 0)
				break;
			samples.insert(samples.end(), block.begin(), block.begin() + n);
		}
		if (samples.empty())
		{
			std::printf("%-17s  empty WAV\n", m.name.c_str());
			anyFail = true;
			++failed;
			continue;
		}

		// Two demodulators: ZC for clean sync-band threshold detection,
		// IQ for unbiased mean frequency.
		const auto zcFreq = demodulate(samples, rate, DemodKind::ZeroCrossing);
		const auto iqFreq = demodulate(samples, rate, DemodKind::Quadrature);

		const std::vector<Run> runs = findSyncRuns(zcFreq);
		const Decoder::VIS vis = Decoder::detectVIS(zcFreq, rate);
		if (!vis.found || runs.size() < 5)
		{
			std::printf("%-17s  header not recognised\n", m.name.c_str());
			anyFail = true;
			++failed;
			continue;
		}

		// Scanline syncs: runs ending past the VIS header. Skip the first
		// (it merges the VIS stop marker with line 0's sync) and the last
		// (the trailing flush of the final line). Period = end-to-end
		// interval of the remaining syncs.
		std::vector<Run> scanSyncs;
		for (const Run &r : runs)
			if (r.end > vis.headerEnd)
				scanSyncs.push_back(r);
		if (scanSyncs.size() < 4)
		{
			std::printf("%-17s  too few scanline syncs (%zu)\n",
			            m.name.c_str(), scanSyncs.size());
			anyFail = true;
			++failed;
			continue;
		}
		double sumPeriod = 0.0;
		int count = 0;
		for (size_t k = 1; k + 1 < scanSyncs.size(); ++k)
		{
			sumPeriod += double(scanSyncs[k + 1].end - scanSyncs[k].end);
			++count;
		}
		const double measuredMs = 1000.0 * (sumPeriod / double(count)) / double(rate);
		const double expectedMs = double(m.linesPerSync) * 60000.0 / double(m.lpm);
		const double errMs = measuredMs - expectedMs;
		const double errPct = 100.0 * errMs / expectedMs;

		// Calibration Grey: a 50 ms window ending 20 ms before the first
		// sync-band run (the 10 ms cal pulse), comfortably inside the
		// first 300 ms Grey tone. Measured on the IQ demod for an
		// unbiased mean.
		float greyFreq = 0.0f;
		const size_t calBegin = runs[0].begin;
		const size_t guard = rate / 50;  // 20 ms
		const size_t window = rate / 20; // 50 ms
		if (calBegin > guard + window)
			greyFreq = meanFreq(iqFreq, calBegin - guard - window, calBegin - guard);

		const bool periodOK = std::abs(errPct) < 0.5;
		const bool greyOK = std::abs(greyFreq - 1900.0f) < 1.0f;
		const bool ok = periodOK && greyOK;
		if (!ok)
			anyFail = true;
		if (ok)
			++passed;
		else
			++failed;

		std::printf("%-17s  %12.3f  %12.3f  %+7.2f%%  %12.4f  %s%s\n",
		            m.name.c_str(),
		            expectedMs, measuredMs, errPct, greyFreq,
		            periodOK ? "" : "[PERIOD] ",
		            greyOK ? "" : "[GREY]");
	}

	std::printf("\nSummary: %d passed, %d failed.\n", passed, failed);
	return anyFail ? 1 : 0;
}
