include $(HCLIB_ROOT)/include/hclib.mak

EXE=pagerank_par
EXE1=pagerank_ser

all: $(EXE) $(EXE1)

pagerank_par: pagerank_par.cpp
	$(CXX) -ggdb -std=c++11  $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

pagerank_ser: pagerank_ser.cpp
	g++ -ggdb -std=c++11  pagerank_ser.cpp -o pagerank_ser

clean:
	rm -rf $(EXE) $(EXE1)

