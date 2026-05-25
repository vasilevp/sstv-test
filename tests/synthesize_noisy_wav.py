"""Synthesize a noisy SSTV WAV for testing the input bandpass prefilter.

Takes the clean outputs/martin1.wav and mixes in:
  - 60 Hz mains hum at ~half signal amplitude (severe out-of-band noise)
  - broadband white hiss
  - a 5 kHz whistle above the SSTV band

The hum and whistle live well outside the 1100..2300 Hz SSTV tone range,
so a well-designed input bandpass should reject them and let the
downstream demodulator decode the signal cleanly.

Run from the project root after building the encoder:

    ./build/encoder colortest.bmp          # produces outputs/martin1.wav
    python3 tests/synthesize_noisy_wav.py  # writes exp/noisy_martin1.wav
"""

import math
import os
import random


def main():
    os.makedirs('exp', exist_ok=True)

    with open('outputs/martin1.wav', 'rb') as f:
        data = f.read()

    header = data[:44]
    samples = bytearray(data[44:])
    n = len(samples)
    fs = 8000

    print(f'samples in martin1.wav: {n:,}')

    sig_amp = 64       # signal sits at about ±64 from 8-bit centre (128)
    hum_amp = 30       # 60 Hz mains hum — ~half signal amplitude (brutal)
    hiss_amp = 12      # broadband white noise
    whistle_amp = 15   # 5 kHz tone above the SSTV band

    random.seed(42)
    out = bytearray(n)
    for i in range(n):
        s = samples[i] - 128
        hum = hum_amp * math.sin(2 * math.pi * 60.0 * i / fs)
        hiss = hiss_amp * (random.random() * 2 - 1)
        whistle = whistle_amp * math.sin(2 * math.pi * 5000.0 * i / fs)
        v = s + hum + hiss + whistle
        v = max(-128, min(127, v))
        out[i] = int(v) + 128

    with open('exp/noisy_martin1.wav', 'wb') as f:
        f.write(header)
        f.write(bytes(out))

    print('wrote exp/noisy_martin1.wav')


if __name__ == '__main__':
    main()
