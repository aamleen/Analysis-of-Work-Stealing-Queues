include $(HCLIB_ROOT)/include/hclib.mak

EXE=kcore_par
EXE1=kcore_ser

all: $(EXE) $(EXE1)

kcore_par: KCore_Par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

kcore_ser: KCore_Ser.cpp
	g++ -std=c++11 -ggdb KCore_Ser.cpp -o kcore_ser

clean:
	rm -rf $(EXE) $(EXE1)

