#include <iostream>
#include <chrono>
#include <climits>
#include <unordered_set>

#include "ECLgraph.h"

struct WorkList
{
    int pos = -1;
    int *nodesInWL;

    WorkList(int num){
        nodesInWL = new int[num];
    }

    int size(){
        return pos+1;
    }

    void add(int node){
        nodesInWL[++pos] = node;
    }

    void reset(){
        pos = -1;
    }

    bool isEmpty(){
        if (pos == -1)
            return true;
        return false;
    }

};

struct BFS
{
    public:
        ECLgraph graph;
        const char* fileName;
        int src;
        int* levels;

    void parseArgs(int argc, char** argv) {
        int i = 0;
        while (i < argc) {
            std::string loopOptionKey = argv[i];
            if (loopOptionKey == "-file"){
                i += 1;
                fileName = argv[i];
            }else if (loopOptionKey == "-s"){
                i += 1;
                src = std::stoi(argv[i]);
            }
            i += 1;
        }
    }

    void initialiseLevels(){
        for (int i = 0; i<graph.nodes; i++){
            levels[i] = INT_MAX;
        }
    }

    void runBFS(){

        levels[src] = 0;
        WorkList *curr = new WorkList(graph.nodes);
        WorkList *next = new WorkList(graph.nodes);

        int nextLevel = 0;

        next->add(src);

        while (!next->isEmpty()){
            std::swap(curr, next);
            next->reset();

            ++nextLevel;

            for (int itr = 0; itr < curr->size(); itr++){
                int node = curr->nodesInWL[itr];
                // std::cout << node  << " ";

                for (int i = graph.nindex[node]; i<graph.nindex[node+1]; i++){
                    int neigh = graph.nlist[i];
                    if (levels[neigh] == INT_MAX){
                        levels[neigh] = nextLevel;
                        next->add(neigh);
                    }
                }
            }
            // std::cout << "\n";

        }
    }


    int main(int argc, char **argv)
    {
        src = 0;
        parseArgs(argc, argv);

        graph = readECLgraph(fileName);

        levels = new int[graph.nodes];

        printf("\nConfigurations: number of nodes- %d, source- %d\n", graph.nodes, src);

        initialiseLevels();

        auto startTime = std::chrono::steady_clock::now();
        runBFS();
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = endTime - startTime;
        
        std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000 <<  " ms \n";

        freeECLgraph(graph);
        delete[] levels;
      
        return 0;

    }
};


int main(int argc, char **argv)
{
    BFS bfs;
    bfs.main(argc, argv);
    return 0;
}
