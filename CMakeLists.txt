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

# --- Libraries -------------------------------------------------------------
# C-Wav-Lib (RIFF/WAV reader-writer), built from the git submodule.
add_library(wav STATIC
  modules/C-Wav-Lib/endianness.c
  modules/C-Wav-Lib/wav.c)
target_include_directories(wav PUBLIC modules)

# LoadBMP wrapper.
add_library(bmp STATIC
  src/bmp/loadbmp.cpp)
target_include_directories(bmp PUBLIC modules)

# --- Executables -----------------------------------------------------------
add_executable(encoder
  src/encoder/main.cpp
  src/encoder/encoder.cpp
  src/encoder/robot8.cpp
  src/encoder/robot.cpp
  src/encoder/martin.cpp
  src/encoder/scottie.cpp
  src/encoder/pd.cpp
  src/encoder/utils.cpp
  src/encoder/wav.cpp)
target_include_directories(encoder PRIVATE src)
target_link_libraries(encoder PRIVATE wav bmp)

add_executable(decoder
  src/decoder/main.cpp
  src/decoder/decoder.cpp
  src/decoder/demodulator.cpp
  src/decoder/zero_crossing_demodulator.cpp
  src/decoder/quadrature_demodulator.cpp
  src/decoder/robot8.cpp
  src/decoder/robot.cpp
  src/decoder/martin.cpp
  src/decoder/scottie.cpp
  src/decoder/pd.cpp)
target_include_directories(decoder PRIVATE src modules)
target_link_libraries(decoder PRIVATE wav bmp)

install(TARGETS encoder decoder RUNTIME DESTINATION bin)
