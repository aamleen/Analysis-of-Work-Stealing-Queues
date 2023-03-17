include $(HCLIB_ROOT)/include/hclib.mak

EXE=pta_par
EXE1=pta_ser

all: $(EXE) $(EXE1)

pta_par: PTA_par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

pta_ser: PTA_ser.cpp
	g++ -std=c++11 -ggdb PTA_ser.cpp -o pta_ser

clean:
	rm -rf $(EXE) $(EXE1)

