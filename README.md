# sstv-test
A Robot 8 B/W SSTV encoder made for shits and giggles.

## How to build and run
```
git clone git@github.com:exploser/sstv-test.git
git submodule update --init --recursive
gcc -c -o endianness.o modules/C-Wav-Lib/endianness.c
gcc -c -o wav.o modules/C-Wav-Lib/wav.c
g++ *.o main.cpp
```
