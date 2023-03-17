include $(HCLIB_ROOT)/include/hclib.mak

EXE=labelPropagation_par
EXE1=labelPropagation_ser

all: $(EXE) $(EXE1)

labelPropagation_par: labelPropagation_par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

labelPropagation_ser: labelPropagation_ser.cpp
	g++ -std=c++11 -ggdb labelPropagation_ser.cpp -o labelPropagation_ser

clean:
	rm -rf $(EXE) $(EXE1)

