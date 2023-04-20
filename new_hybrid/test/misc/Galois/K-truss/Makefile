include $(HCLIB_ROOT)/include/hclib.mak

EXE=ktruss_par
EXE1=ktruss_ser

all: $(EXE) $(EXE1)

ktruss_par: k-truss_Par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

ktruss_ser: k-truss_Ser.cpp
	g++ -std=c++11 -ggdb k-truss_Ser.cpp -o ktruss_ser

clean:
	rm -rf $(EXE) $(EXE1)

