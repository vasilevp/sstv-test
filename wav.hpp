#pragma once
#include <vector>
#include <string>

extern "C" {
#include "C-Wav-Lib/wav.h"
};

class WAV
{
    const size_t sample_rate;

    FILE *f = nullptr;

    std::vector<uint8_t> samples = std::vector<uint8_t>(0, 0);
    wav_sample_t sample = {1, 1, 1, nullptr};

  public:
    WAV(const std::string &name, const size_t sample_rate)
        : sample_rate(sample_rate)
    {
        f = name != "-" ? fopen(name.c_str(), "wb") : stdout;
        samples.reserve(sample_rate);
    }

    ~WAV()
    {
        write_wav_header(
            f,
            1,
            sample_rate,
            1,
            samples.size() + sample_rate / 10 * 2);

        std::vector<uint8_t> silence(sample_rate / 10, 128);

        // surround actual data with silence for easier playback
        sample.DataSize = silence.size();
        sample.sampleData = silence.data();
        write_wav_sample(f, &sample);

        sample.DataSize = samples.size();
        sample.sampleData = samples.data();
        write_wav_sample(f, &sample);

        sample.DataSize = silence.size();
        sample.sampleData = silence.data();
        write_wav_sample(f, &sample);

        fclose(f);
    }

    void put(uint8_t sample)
    {
        // don't care about RAM, care about speed
        if (samples.capacity() == samples.size())
            samples.reserve(samples.capacity() * 2);

        samples.push_back(sample);
    }

    size_t size() const
    {
        return samples.size();
    }
};
