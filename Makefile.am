bin_PROGRAMS = sstv

sstv_SOURCES = src/encoder/main.cpp \
			   src/encoder/encoder.cpp \
			   src/encoder/loadbmp.cpp \
			   src/encoder/robot8.cpp \
			   src/encoder/robot.cpp \
			   src/encoder/martin.cpp \
			   src/encoder/scottie.cpp

# Enable C++23 and C23 standards
AM_CXXFLAGS = --std=c++23
AM_CFLAGS = --std=c23
# Add include paths for libraries
AM_CPPFLAGS = -O3 -flto -I$(srcdir)/src -I$(builddir) -I$(srcdir)/modules

# Build a static library for modules/C-Wav-Lib
noinst_LIBRARIES = libwav.a
libwav_a_SOURCES = modules/C-Wav-Lib/endianness.c modules/C-Wav-Lib/wav.c

# Link the static library with the sstv program
sstv_LDADD = libwav.a

# Specify the directory for object files
DEPDIR = $(srcdir)/.deps

# Integrate clang-tidy
CLANG_TIDY = clang-tidy
CLANG_TIDY_CHECKS = -checks=-*,modernize-*,performance-*,bugprone-*,clang-analyzer-*,misc-*,portability-*,llvm-*,cert-*,concurrency-*,-misc-const-correctness

# Define a rule for each source file
.PHONY: tidy
tidy: $(sstv_SOURCES:.cpp=.tidy)

.PHONY: %.tidy
%.tidy: %.cpp
	$(CLANG_TIDY) $(CLANG_TIDY_CHECKS) $< -- $(AM_CPPFLAGS) $(AM_CXXFLAGS)
	@echo "clang-tidy check completed for $<"
	@echo "----------------------------------------"
