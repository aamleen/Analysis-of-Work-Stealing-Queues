// Certain sections and idea has been picked from Galois

#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <cstring>

int seed = 0;

class Constraint
{
public:
  using ConstraintType = enum { AddressOf = 0, Copy, Load, Store };

private:
  unsigned src;
  unsigned dst;
  ConstraintType type;

public:
  Constraint(ConstraintType tt, unsigned ss, unsigned dd) {
    src  = ss;
    dst  = dd;
    type = tt;
  }

  std::pair<unsigned, unsigned> getSrcDst() const {
    return std::pair<unsigned, unsigned>(src, dst);
  }

  ConstraintType getType() const { return type; }

  void print() const {
    if (type == Store) {
      std::cerr << "*";
    }

    std::cerr << "v" << dst;
    std::cerr << " = ";

    if (type == Load) {
      std::cerr << "*";
    } else if (type == AddressOf) {
      std::cerr << "&";
    }

    std::cerr << "v" << src;

    std::cerr << std::endl;
  }
};

struct WorkList{
	int pos = -1;
	int *worklist;
	int *nodesPresent;

	WorkList(int num){
		worklist = new int[num];
		nodesPresent = new int[num+1];
	}
};

struct PointsToAnalysis
{
    using PointsToConstraints = std::vector<Constraint>;
    protected:
        PointsToConstraints addressCopyConstraints;
        PointsToConstraints loadStoreConstraints;
    public:
        int numNodes;
        int nconstraints;
        int *graph;
        int *variableSets;
        WorkList *initialWorklist;

    const char* parseArgs(int argc, const char** argv) {
        int i = 0;
        while (i < argc) {
            std::string loopOptionKey = argv[i];
            if (loopOptionKey == "-file"){
                i += 1;
                return argv[i];
            }
            i += 1;
        }

        fprintf(stderr, "Error: Mandatory option -file not provided\n");
        exit(1);
    }

    void printConstraints(PointsToConstraints& constraints) {
        for (auto ii = constraints.begin(); ii != constraints.end(); ++ii) {
          ii->print();
        }
    }

    unsigned readConstraints(const char* file) {

        std::ifstream cfile(file);
        std::string cstr;

        getline(cfile, cstr); // # of vars.
        sscanf(cstr.c_str(), "%d", &numNodes);

        getline(cfile, cstr); // # of constraints.
        sscanf(cstr.c_str(), "%d", &nconstraints);

        addressCopyConstraints.clear();
        loadStoreConstraints.clear();

        unsigned constraintNum;
        unsigned src;
        unsigned dst;
        unsigned offset;

        Constraint::ConstraintType type;

        for (unsigned ii = 0; ii < nconstraints; ++ii) {
          getline(cfile, cstr);
          union {
            int as_int;
            Constraint::ConstraintType as_ctype;
          } type_converter;
          sscanf(cstr.c_str(), "%d,%d,%d,%d,%d", &constraintNum, &src, &dst,
                 &type_converter.as_int, &offset);

          type = type_converter.as_ctype;

          Constraint cc(type, src, dst);

          if (type == Constraint::AddressOf || type == Constraint::Copy) {
            addressCopyConstraints.push_back(cc);
          } else if (type == Constraint::Load || type == Constraint::Store) {
              loadStoreConstraints.push_back(cc);
          }
        }

        cfile.close();

        // printConstraints(addressCopyConstraints);
        // printConstraints(loadStoreConstraints);

        return numNodes;
    }

    bool checkIfEmpty(int source){
        for (int i = 1; i<numNodes; i++){
            if (variableSets[source*(numNodes+1) + i] == 1) return false;
        }
        return true;
    }

    void constructInitialGraphAndSets(){

        graph = new int[(numNodes+1)*(numNodes+1)];
        variableSets = new int[(numNodes+1)*(numNodes+1)];

        // for (int i=1;i<=numNodes;i++){
        //     graph[i] = new int[numNodes+1];
        //     variableSets[i] = new int[numNodes+1];
        //     memset(graph[i], 0, numNodes+1);
        //     memset(variableSets[i], 0, numNodes+1);
        // }

        for (std::vector<Constraint>::iterator it = addressCopyConstraints.begin(); it != addressCopyConstraints.end(); ++it) {
            unsigned src = it->getSrcDst().first;
            unsigned dst = it->getSrcDst().second;
            if (it->getType() == 1){
                graph[src*(numNodes+1) + dst] = 1;
            }else{
                variableSets[dst*(numNodes+1) + src] = 1;
            }
        }
    }

    void initialiseWorklist(){

        initialWorklist = new WorkList(numNodes);

        for (std::vector<Constraint>::iterator it = addressCopyConstraints.begin(); it != addressCopyConstraints.end(); ++it) {
            unsigned src = it->getSrcDst().first;
            unsigned dst = it->getSrcDst().second;

            // it->print();
            if (initialWorklist->nodesPresent[dst]!=1 && !checkIfEmpty(dst)){
                initialWorklist->worklist[++initialWorklist->pos] = dst;
                initialWorklist->nodesPresent[dst] = 1;
            }
        }

    }

    bool unionOfSets(int toUpdate, int other){
        
        bool updated = false;

        for (int i = 1; i<=numNodes; i++){
            if (variableSets[other*(numNodes+1) + i] == 1){
                if (variableSets[toUpdate*(numNodes+1) + i] != 1){
                    variableSets[toUpdate*(numNodes+1) + i] = 1;
                    updated = true;
                }
            }
        }

        return updated;
    }

    void calculateTransitiveClosure(){
    	WorkList *secondaryWorklist = new WorkList(numNodes);

        while (initialWorklist->pos != -1){
            // printf("%d %d\n", initialWorklist->pos, secondaryWorklist->pos);
            
            int count = initialWorklist->pos;
            for (int nodeItr = 0; nodeItr<=count; nodeItr++){
            // printf("nodeItr %d, %d\n", nodeItr, initialWorklist->pos);
            int n = initialWorklist->worklist[nodeItr];
            initialWorklist->pos -= 1;
            initialWorklist->nodesPresent[n] = 0;

            for (int i = 1; i<numNodes+1; i++){
                if (variableSets[n*(numNodes+1) + i] == 1){
                    for (std::vector<Constraint>::iterator it = loadStoreConstraints.begin(); it != loadStoreConstraints.end(); ++it) {
                        if (it->getSrcDst().first == n){
                            if (it->getType() == 3){
                                if (graph[i*(numNodes+1) + it->getSrcDst().second] != 1){
                                    graph[i*(numNodes+1) + it->getSrcDst().second] = 1;
                                    if (secondaryWorklist->nodesPresent[i] != 1){
                                        secondaryWorklist->worklist[++secondaryWorklist->pos] = i;
                                        secondaryWorklist->nodesPresent[i] = 1;
                                    }
                                }
                            }
                        }else if (it->getSrcDst().second == n){
                            if (it->getType() == 4){
                                if (graph[it->getSrcDst().first*(numNodes+1) + i] != 1){
                                    graph[it->getSrcDst().first*(numNodes+1) + i] = 1;
                                    if (secondaryWorklist->nodesPresent[it->getSrcDst().first] != 1){
                                        secondaryWorklist->worklist[++secondaryWorklist->pos] = it->getSrcDst().first;
                                        secondaryWorklist->nodesPresent[it->getSrcDst().first] = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            for (int i = 1; i<=numNodes; i++){
                if (graph[n*(numNodes+1) + i] == 1){

                    bool changed = unionOfSets(i, n);
                    if (changed){
                        if (secondaryWorklist->nodesPresent[i] != 1){
                            secondaryWorklist->worklist[++secondaryWorklist->pos] = i;
                            secondaryWorklist->nodesPresent[i] = 1;
                        }
                    }
                }
            }
        }

            std::swap(initialWorklist, secondaryWorklist);
        }

        delete initialWorklist->worklist;
        delete initialWorklist->nodesPresent;
        delete secondaryWorklist->worklist;
        delete secondaryWorklist->nodesPresent;
        delete initialWorklist;
        delete secondaryWorklist;
    }

    void printGraph(){
        for (int i = 1; i<=numNodes; i++){
            std::cout << i << "--> ";
            for (int j = 0; j<=numNodes; j++){
                if (graph[i*(numNodes+1) + j] == 1){
                    std::cout << j << ", ";
                }
            }
            std::cout << "\n";
        }
    }

    void printPointsToSet(){
        for (int i = 1; i<=numNodes; i++){
            std::cout << i << "--> ";
            for (int j = 0; j<=numNodes; j++){
                if (variableSets[i*(numNodes+1) + j] == 1){
                    std::cout << j << ", ";
                }
            }
            std::cout << "\n";
        }
    }

    int main(int argc, char const *argv[])
    {
        const char* filename = parseArgs(argc, argv);
        unsigned nodes = readConstraints(filename);
        constructInitialGraphAndSets();
        initialiseWorklist();
        printf("Starting Analysis\n");
        auto startTime = std::chrono::steady_clock::now();
        calculateTransitiveClosure();
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = endTime - startTime;
        std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000 <<  " ms \n";
        
        delete graph;
        delete variableSets;
        
        return 0;
    }
};

int main(int argc, char const *argv[])
{
    PointsToAnalysis Pta;
    Pta.main(argc, argv);
    return 0;
}
