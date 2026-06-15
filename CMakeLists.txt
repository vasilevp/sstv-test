cmake_minimum_required(VERSION 3.25)
project(sstv-test LANGUAGES C CXX)

# Default to a Debug build (AddressSanitizer) when no build type is given.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
               Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Link-time optimization (Release builds) -------------------------------
# Applied as explicit flags rather than CMake's IPO support: IPO relies on a
# plugin-aware archiver (gcc-ar), which is not discoverable on this system.
# -ffat-lto-objects keeps a real symbol table in each object alongside the LTO
# IR, so the plain archiver can still index the static libraries.
add_compile_options("$<$<CONFIG:Release>:-flto;-ffat-lto-objects>")
add_link_options("$<$<CONFIG:Release>:-flto>")

# --- AddressSanitizer (Debug builds) ---------------------------------------
add_compile_options(
  "$<$<CONFIG:Debug>:-fsanitize=address;-fno-omit-frame-pointer>")
add_link_options("$<$<CONFIG:Debug>:-fsanitize=address>")

# --- Optional clang-tidy ---------------------------------------------------
option(ENABLE_CLANG_TIDY "Run clang-tidy during the build" OFF)
if(ENABLE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY
      clang-tidy
      "-checks=-*,modernize-*,performance-*,bugprone-*,clang-analyzer-*,misc-*,portability-*,llvm-*,cert-*,concurrency-*,-misc-const-correctness")
endif()

# --- Executables -----------------------------------------------------------
add_executable(encoder
  src/encoder/main.cpp
  src/encoder/encoder.cpp
  src/encoder/bmp_row_source.cpp
  src/encoder/wav_sample_sink.cpp
  src/encoder/robot8.cpp
  src/encoder/robot.cpp
  src/encoder/martin.cpp
  src/encoder/scottie.cpp
  src/encoder/pd.cpp
  src/encoder/utils.cpp)
target_include_directories(encoder PRIVATE src modules)

add_executable(decoder
  src/decoder/main.cpp
  src/decoder/decoder.cpp
  src/decoder/bmp_row_sink.cpp
  src/decoder/demodulator.cpp
  src/decoder/zero_crossing_demodulator.cpp
  src/decoder/quadrature_demodulator.cpp
  src/decoder/bandpass_demodulator.cpp
  src/decoder/biquad.cpp
  src/decoder/kaiser_fir.cpp
  src/decoder/schmitt_trigger.cpp
  src/decoder/exponential_moving_average.cpp
  src/decoder/robot8.cpp
  src/decoder/robot.cpp
  src/decoder/martin.cpp
  src/decoder/scottie.cpp
  src/decoder/pd.cpp)
target_include_directories(decoder PRIVATE src)

install(TARGETS encoder decoder RUNTIME DESTINATION bin)

# Correctness test for the demodulators. Stand-alone executable that feeds
# pure sines at known frequencies through both demodulators and prints
# accuracy statistics. Not registered with CTest; run by hand.
add_executable(demodulator_correctness
  tests/demodulator_correctness.cpp
  src/decoder/demodulator.cpp
  src/decoder/zero_crossing_demodulator.cpp
  src/decoder/quadrature_demodulator.cpp
  src/decoder/bandpass_demodulator.cpp
  src/decoder/biquad.cpp
  src/decoder/kaiser_fir.cpp)
target_include_directories(demodulator_correctness PRIVATE src/decoder)

# Correctness test for the encoder (the modulator). Reads each WAV in
# outputs/ and checks its scanline cadence and the 1900 Hz calibration
# Grey tone against the SSTV Handbook (Bruchanov ch. 5, sstv-handbook.com).
# Run after the encoder has been run on colortest.bmp.
add_executable(modulator_correctness
  tests/modulator_correctness.cpp
  src/decoder/decoder.cpp
  src/decoder/demodulator.cpp
  src/decoder/zero_crossing_demodulator.cpp
  src/decoder/quadrature_demodulator.cpp
  src/decoder/bandpass_demodulator.cpp
  src/decoder/biquad.cpp
  src/decoder/kaiser_fir.cpp
  src/decoder/schmitt_trigger.cpp
  src/decoder/exponential_moving_average.cpp)
target_include_directories(modulator_correctness PRIVATE src/decoder)
