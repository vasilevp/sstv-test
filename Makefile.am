bin_PROGRAMS = sstv
sstv_SOURCES = main.cpp loadbmp.cpp encoder.cpp robot8.cpp robot.cpp martin.cpp scottie.cpp
AM_CXXFLAGS = --std=c++23
AM_CFLAGS = --std=c23

# Build a static library for modules/C-Wav-Lib
noinst_LIBRARIES = libwav.a
libwav_a_SOURCES = modules/C-Wav-Lib/endianness.c modules/C-Wav-Lib/wav.c

# Link the static library with the sstv program
sstv_LDADD = libwav.a

# Specify the directory for object files
DEPDIR = $(srcdir)/.deps

# Add include paths for libraries
AM_CPPFLAGS = -I$(srcdir) -I$(builddir) -I$(srcdir)/modules

# Integrate clang-tidy
CLANG_TIDY = clang-tidy
CLANG_TIDY_CHECKS = -checks=-*,modernize-*,performance-*,bugprone-*,clang-analyzer-*,misc-*,portability-*,llvm-*,cert-*,concurrency-*,-misc-const-correctness

# Define a rule for each source file
.PHONY: tidy
tidy: $(sstv_SOURCES:.cpp=.tidy)

.PHONY: %.tidy
%.tidy: %.cpp
	$(CLANG_TIDY) $(CLANG_TIDY_CHECKS) $< -- $(AM_CPPFLAGS) $(AM_CXXFLAGS)

CLEANFILES = *.o *.exe *.tidy

distclean-local:
	rm -rf aclocal.m4 autom4te.cache config.log config.status configure depcomp install-sh Makefile.in missing compile
