#include <cstdint>
#include <iostream>
#include <memory>
#include <functional>
#include <cmath>
#include <vector>

extern "C" {
#include "C-Wav-Lib/wav.h"
};

#define LOADBMP_IMPLEMENTATION
#include "LoadBMP/loadbmp.h"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define DEFER(x) std::shared_ptr<void> CONCAT(_defer___, __COUNTER__)(nullptr, [&](auto) { x; });
#define LEN(x) (sizeof(x) / sizeof(*x))

static int8_t lut[] = {
	0,4,8,12,16,20,24,28,32,35,39,43,47,50,
	54,58,61,65,68,71,75,78,81,84,87,90,93,
	95,98,100,103,105,107,109,111,113,115,
	117,118,119,121,122,123,124,125,125,126,
	126,127,127,127,127,127,126,126,125,125,
	124,123,122,121,119,118,117,115,113,111,
	109,107,105,103,100,98,95,93,90,87,84,81,
	78,75,71,68,65,61,58,54,50,47,43,39,35,32,
	28,24,20,16,12,8,4,0,-4,-8,-12,-16,-20,-24,
	-28,-32,-35,-39,-43,-47,-50,-54,-58,-61,-65,
	-68,-71,-75,-78,-81,-84,-87,-90,-93,-95,-98,
	-100,-103,-105,-107,-109,-111,-113,-115,-117,
	-118,-119,-121,-122,-123,-124,-125,-125,-126,
	-126,-127,-127,-127,-127,-127,-126,-126,-125,
	-125,-124,-123,-122,-121,-119,-118,-117,-115,
	-113,-111,-109,-107,-105,-103,-100,-98,-95,-93,
	-90,-87,-84,-81,-78,-75,-71,-68,-65,-61,-58,-54,
	-50,-47,-43,-39,-35,-32,-28,-24,-20,-16,-12,-8,-4,
};

class WAV
{
	const size_t sample_rate;

	FILE *f = nullptr;
	size_t N = 0;

	std::vector<uint8_t> samples = std::vector<uint8_t>(0, 0);
	wav_sample_t sample = {1, 1, 1, nullptr};

  public:
	WAV(const char *name, const size_t sample_rate)
		: sample_rate(sample_rate)
	{
		f = fopen(name, "wb");
		samples.reserve(sample_rate);
	}

	void put(uint8_t sample)
	{
		// don't care about RAM, care about speed
		if (samples.capacity() == samples.size())
			samples.reserve(samples.capacity() * 2);

		samples.push_back(sample);
		++N;
	}

	~WAV()
	{
		write_wav_header(
			f,
			1,
			sample_rate,
			1,
			N);

		sample.DataSize = N;
		sample.sampleData = samples.data();
		write_wav_sample(f, &sample);

		fclose(f);
	}
};

void wiggle(uintmax_t &freq)
{
	const uintmax_t freq_hi = 2200;
	const uintmax_t freq_lo = 1200;
	freq = (freq == freq_lo ? freq_hi : freq_lo);
}

size_t ms2samp(float ms, size_t sample_rate)
{
	return sample_rate * ms / 1000;
}

#define SYNTH(length, freq)                        \
    {                                              \
        auto frame = ms2samp(length, sample_rate); \
        while (frame--)                            \
        {                                          \
            uint8_t x = lut[idx % LEN(lut)] + 127; \
            idx += freq / freq_step;               \
            w.put(x);                              \
        }                                          \
    }

void sstv(char *file)
{
	// load BMP
	uint8_t *pixels = NULL;
	uint32_t width, height;
	auto err = loadbmp_decode_file(file, &pixels, &width, &height, LOADBMP_RGB);
	DEFER(free(pixels));

	if (err || width != 120 || height != 120)
	{
		throw std::runtime_error("Image must be 120x120 pixels!");
	}

	const size_t sample_rate = 6000;

	WAV w("sstv.wav", sample_rate);

	const int freq_step = sample_rate / LEN(lut);

	int idx = 0;

	// sstv header
	// calibration pulse
	SYNTH(300, 1900);
	SYNTH(10, 1200);
	SYNTH(300, 1900);

	// VIS code (all zeros because documentation on this sucks balls)
	// start/stop marker
	SYNTH(30, 1200);

	// 7 zero bits
	for (int i = 0; i < 7; ++i)
	{
		// 1300 = 0, 1200 = 1
		SYNTH(30, 1300);
	}
	
	// parity bit
	SYNTH(30, 1300);
	// start/stop marker
	SYNTH(30, 1200);

	for (int i = 0; i < 120; ++i)
	{
		// sync pulse
		SYNTH(7, 1200);

		// 20 to 140th pixels: a very dirty filthy fucking hack because reasons
		for (int j = 20; j < 140; ++j)
		{
			size_t offset = (i * 120 + j) * 3;
			float val = float(pixels[offset] + pixels[offset + 1] + pixels[offset + 2]) / 3;

			float freq = 1500 + 800 * val / 255;

			// pixel
			SYNTH(0.5, freq);
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		throw std::runtime_error(std::string("Usage: ") + argv[0] + " input_file.bmp");
	}

	sstv(argv[1]);
	/*
	const size_t sample_rate = 20000;

	wav w("out.wav", sample_rate);

	const int freq_step = sample_rate / LEN(lut);

	int idx = 0;

	const float baud_step = float(sample_rate) / 1200;

	std::string message = "Lorem ipsum dolor sit amet, ferri aperiri his ad, corpora sapientem ea vix";

	uintmax_t freq = 1200;

	size_t sample = 0;

	int total_bits = 0;

	int ones = 0;

	for (uint16_t byte : message)
	{
		for (int bit = 1; bit <= 8; ++bit)
		{
			++total_bits;

			// NRZI encoding
			if ((byte & 0x01) == 0)
			{
				// wiggle frequency if bit is unset
				wiggle(freq);
			}
			else if (++ones == 6)
			{
				wiggle(freq);
				ones = 0;
				byte <<= 1; // holy fuck knuckles
			}

			byte >>= 1;

			// write samples
			while (sample++ < total_bits * baud_step)
			{
				uint8_t x = lut[idx % LEN(lut)] + 127;
				idx += freq / freq_step;
				w.put(x);
			}
		}
	}*/
}