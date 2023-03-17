include $(HCLIB_ROOT)/include/hclib.mak
include $(HCLIB_ROOT)/include/include_GCC.mk

LIKWID_INCLUDE ?= -I/usr/local/include
LIKWID_LIBDIR ?= -L/usr/local/lib
LIKWID_LIB ?= -llikwid

EXE=UTS

all: clean $(EXE) clean-obj

UTS: 
	$(CXX) -O3 -DBRG_RNG -Wno-write-strings $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) $(LIKWID_INCLUDE) $(LIKWID_LIBDIR) -o $@ UTS.cpp rng/brg_sha1.c uts.c $(PROJECT_LDLIBS) $(LIKWID_LIB)

clean-obj:
	rm -rf *.o *.dSYM

clean:
	rm -rf *.o $(EXE) *.dSYM
