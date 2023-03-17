include $(HCLIB_ROOT)/include/hclib.mak

EXE=independentset_par
EXE1=independentset_ser

all: $(EXE) $(EXE1)

independentset_par: IndependentSet_Par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

independentset_ser: IndependentSet_Ser.cpp
	g++ -I /usr/local/include -std=c++11  -ggdb IndependentSet_Ser.cpp -o independentset_ser

clean:
	rm -rf $(EXE) $(EXE1)
