include $(HCLIB_ROOT)/include/hclib.mak

EXE=boruvka_par
EXE1=boruvka_ser

all: $(EXE) $(EXE1)

boruvka_par: Boruvka_Par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

boruvka_ser: Boruvka_Ser.cpp
	g++ -std=c++11 -ggdb Boruvka_Ser.cpp -o boruvka_ser

clean:
	rm -rf $(EXE) $(EXE1)

