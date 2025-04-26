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
CLANG_TIDY_CHECKS = -checks=-*,modernize-*,readability-*

# Run clang-tidy on source files
tidy:
	$(CLANG_TIDY) $(CLANG_TIDY_CHECKS) $(sstv_SOURCES) -- $(AM_CPPFLAGS) $(AM_CXXFLAGS)

CLEANFILES = build/*.o *.exe

distclean-local:
	rm -rf aclocal.m4 autom4te.cache config.log config.status configure depcomp install-sh Makefile.in missing compile
