#include "demodulator.hpp"

#include "quadrature_demodulator.hpp"
#include "zero_crossing_demodulator.hpp"

const char *demodKindName(DemodKind kind)
{
	switch (kind)
	{
	case DemodKind::ZeroCrossing: return "zero-crossing";
	case DemodKind::Quadrature:   return "quadrature";
	}
	return "unknown";
}

std::unique_ptr<Demodulator> makeDemodulator(DemodKind kind, uint32_t sampleRate)
{
	switch (kind)
	{
	case DemodKind::ZeroCrossing:
		return std::make_unique<ZeroCrossingDemodulator>(sampleRate);
	case DemodKind::Quadrature:
		return std::make_unique<QuadratureDemodulator>(sampleRate);
	}
	return std::make_unique<ZeroCrossingDemodulator>(sampleRate);
}
