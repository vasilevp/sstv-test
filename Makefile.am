# Enable C++23 and C23 standards
AM_CXXFLAGS = --std=c++23 -flto
AM_CFLAGS = --std=c23 -flto
AM_LDFLAGS = -flto -lunwind -L/nix/store/lfz2hjp4zb4l4pbpcq7n7mnsalqd0ha1-libunwind-1.8.1/lib/
CXXFLAGS = -g -O3 -fsanitize=address
CFLAGS = -g -O3
# Add include paths for libraries
AM_CPPFLAGS = -O3 -I$(srcdir)/src -I$(builddir) -I$(srcdir)/modules

# Libraries
noinst_LIBRARIES = libs/libwav.a libs/libbmp.a
libs_libwav_a_SOURCES = modules/C-Wav-Lib/endianness.c modules/C-Wav-Lib/wav.c
libs_libbmp_a_SOURCES = src/bmp/loadbmp.cpp

# Programs
bin_PROGRAMS = encoder decoder

# Encoder
encoder_SOURCES = src/encoder/main.cpp \
			   src/encoder/encoder.cpp \
			   src/encoder/robot8.cpp \
			   src/encoder/robot.cpp \
			   src/encoder/martin.cpp \
			   src/encoder/scottie.cpp \
			   src/encoder/utils.cpp

encoder_LDADD = libs/libwav.a libs/libbmp.a

# Decoder
decoder_SOURCES = src/decoder/main.cpp \
			   src/decoder/decoder.cpp


# Specify the directory for object files
DEPDIR = $(srcdir)/.deps

# Integrate clang-tidy
CLANG_TIDY = clang-tidy
CLANG_TIDY_CHECKS = -checks=-*,modernize-*,performance-*,bugprone-*,clang-analyzer-*,misc-*,portability-*,llvm-*,cert-*,concurrency-*,-misc-const-correctness

# Define a rule for each source file
.PHONY: tidy
tidy: $(encoder_SOURCES:.cpp=.tidy)

.PHONY: %.tidy
%.tidy: %.cpp
	$(CLANG_TIDY) $(CLANG_TIDY_CHECKS) $< -- $(AM_CPPFLAGS) $(AM_CXXFLAGS)
	@echo "clang-tidy check completed for $<"
	@echo "----------------------------------------"
