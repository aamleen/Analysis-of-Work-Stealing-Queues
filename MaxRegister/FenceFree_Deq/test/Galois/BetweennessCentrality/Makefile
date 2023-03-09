include $(HCLIB_ROOT)/include/hclib.mak

EXE=bc_par
EXE1=bc_ser

all: $(EXE) $(EXE1)

bc_par: BC_par.cpp
	$(CXX) -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

bc_ser: BC_ser.cpp
	g++ -std=c++11 -ggdb BC_ser.cpp -o bc_ser

clean:
	rm -rf $(EXE) $(EXE1)

