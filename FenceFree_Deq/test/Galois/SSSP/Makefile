include $(HCLIB_ROOT)/include/hclib.mak

EXE=sssp_par
EXE1=sssp_ser

all: $(EXE) $(EXE1)

sssp_par: par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

sssp_ser: ser.cpp
	g++ -std=c++11 -ggdb ser.cpp -o sssp_ser

clean:
	rm -rf $(EXE) $(EXE1)

