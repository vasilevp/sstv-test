#pragma once
#include <string>

#include "utils.hpp"
#include "wav.hpp"

class AFSK1200
{
  public:
    static void Encode(const std::string &message, const std::string &out)
    {
        const size_t sample_rate = utils::lut_size * 20;
        const int freq_step = sample_rate / utils::lut_size;
        const float baud_step = float(sample_rate) / 1200;

        int idx = 0;
        uintmax_t freq = 1200;
        int total_bits = 0;
        int ones = 0;

        WAV w(out, sample_rate);
        for (uint16_t byte : message)
        {
            bool separator = byte == 0x7E;

            for (int bit = 1; bit <= 8; ++bit)
            {
                ++total_bits;

                if (!separator && ++ones == 6)
                {
                    ones = 0;
                    byte <<= 1; // holy fuck-knuckles
                    --bit;
                }

                // NRZI encoding
                if ((byte & 0x01) == 0)
                {
                    // wiggle frequency if bit is unset
                    wiggle(freq);
                    ones = 0;
                }

                byte >>= 1;

                // write samples
                while (w.size() < total_bits * baud_step)
                {
                    // do a LUT lookup
                    uint8_t x = 0.5 * utils::lut[(idx / freq_step) % utils::lut_size] + 128;
                    // write sample
                    w.put(x);
                    // advance in LUT
                    idx += freq;
                }
            }
        }
    }

  private:
    static void wiggle(uintmax_t &freq)
    {
        const uintmax_t freq_hi = 2200;
        const uintmax_t freq_lo = 1200;
        freq = (freq == freq_lo ? freq_hi : freq_lo);
    }
};