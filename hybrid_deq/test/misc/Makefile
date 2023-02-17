include $(HCLIB_ROOT)/include/hclib.mak
include $(HCLIB_ROOT)/include/include_GCC.mk

LIKWID_INCLUDE ?= -I/usr/local/include
LIKWID_LIBDIR ?= -L/usr/local/lib
LIKWID_LIB ?= -llikwid

TARGETS := trans_closure qsort fib nqueens heat matmul cilksort srad sor1d

all: clean $(TARGETS) clean-obj

%: %.cpp
	$(CXX) -g -rdynamic -O3 $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) $(LIKWID_INCLUDE) $(LIKWID_LIBDIR) -o $@ $^  $(PROJECT_LDLIBS) $(LIKWID_LIB)

clean-obj:
	rm -rf *.o

clean:
	rm -rf *.o $(TARGETS)
