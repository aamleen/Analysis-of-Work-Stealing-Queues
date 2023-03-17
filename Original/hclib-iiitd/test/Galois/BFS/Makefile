include $(HCLIB_ROOT)/include/hclib.mak

EXE=bfs_par
EXE1=bfs_ser

all: $(EXE) $(EXE1)

bfs_par: bfs_par.cpp
	$(CXX) -std=c++11 -ggdb $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

bfs_ser: bfs_ser.cpp
	g++ -std=c++11 -ggdb bfs_ser.cpp -o bfs_ser

clean:
	rm -rf $(EXE) $(EXE1)

