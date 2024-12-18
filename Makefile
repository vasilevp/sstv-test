CXX=g++
CC=gcc
ECHO=@echo " :: building $@ (depends on $^)..."
CFLAGS=--std=c23
CXXFLAGS=--std=c++23

SUFFIXES := $(SUFFIXES) .hpp
.SUFFIXES:
.SUFFIXES: $(SUFFIXES)

all: sstv

clean:
	${ECHO}
	rm -f *.o *.exe

endianness.o: C-Wav-Lib/endianness.c C-Wav-Lib/endianness.h
	${ECHO}
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o endianness.o C-Wav-Lib/endianness.c

wav.o: C-Wav-Lib/*
	${ECHO}
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o wav.o C-Wav-Lib/wav.c

main.o:	main.cpp *.hpp
	${ECHO}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp

sstv: main.o wav.o endianness.o loadbmp.o encoder.o robot8.o robot.o martin.o scottie.o
	${ECHO}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^
