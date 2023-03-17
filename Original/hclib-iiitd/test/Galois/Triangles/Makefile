include $(HCLIB_ROOT)/include/hclib.mak

EXE=triangle_par
EXE1=triangle_ser

all: $(EXE) $(EXE1)

triangle_par: Triangles_Par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

triangle_ser: Triangles_Ser.cpp
	g++ -std=c++11 -ggdb Triangles_Ser.cpp -o triangle_ser

clean:
	rm -rf $(EXE) $(EXE1)

