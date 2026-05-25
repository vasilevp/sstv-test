#pragma once

// Second-order IIR biquad in transposed direct form II — cheap, branch-free,
// numerically robust in single precision. Coefficients are configured at
// construction (or via setLowpass / setBandpass) and held constant; only z1
// and z2 update per sample.
//
// Reused by every filter in the decoder: the low-pass stages in the
// quadrature demodulator and the input bandpass in BandpassDemodulator.
struct Biquad
{
	// Normalised coefficients (a0 has been divided through).
	float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
	float a1 = 0.0f, a2 = 0.0f;
	// Filter state.
	float z1 = 0.0f, z2 = 0.0f;

	// Audio EQ Cookbook (Robert Bristow-Johnson) low-pass; Q = 1/sqrt(2)
	// gives a maximally-flat Butterworth shape.
	void setLowpass(float cutoffHz, float sampleRate, float Q = 0.7071f);

	// Audio EQ Cookbook high-pass; defaults to Butterworth Q.
	void setHighpass(float cutoffHz, float sampleRate, float Q = 0.7071f);

	// Audio EQ Cookbook bandpass (constant 0 dB peak gain). centerHz is the
	// geometric centre of the passband; Q = centerHz / bandwidthHz.
	void setBandpass(float centerHz, float sampleRate, float Q);

	float process(float x);
};
