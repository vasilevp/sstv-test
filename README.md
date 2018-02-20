# sstv-test
A Robot 8 B/W SSTV encoder made for shits and giggles.

## How to build and run
```
git clone git@github.com:exploser/sstv-test.git
git submodule update --init --recursive
gcc -c -o endianness.o C-Wav-Lib/endianness.c
gcc -c -o wav.o C-Wav-Lib/wav.c
g++ endianness.o wav.o main.cpp
```
```
./a.out <input_file.bmp>
```
