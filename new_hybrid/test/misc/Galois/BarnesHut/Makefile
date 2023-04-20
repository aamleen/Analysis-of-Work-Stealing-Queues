include $(HCLIB_ROOT)/include/hclib.mak

EXE=barnes_par
EXE1=barnes_ser

all: $(EXE) $(EXE1)

barnes_par: BarnesHut_Par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

barnes_ser: BarnesHut_Ser.cpp
	g++ -std=c++11 -ggdb BarnesHut_Ser.cpp -o barnes_ser

clean:
	rm -rf $(EXE) $(EXE1)

