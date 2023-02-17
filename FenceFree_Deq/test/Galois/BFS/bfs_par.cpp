#include <iostream>
#include <chrono>
#include <climits>
#include <unordered_set>
#include <cstring>
#include "hclib.hpp"

#include "ECLgraph.h"

int FACTOR = 1;

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
            }else if (loopOptionKey == "-FACTOR"){
                i += 1;
                FACTOR = std::stoi(argv[i]);
            }
            i += 1;
        }
    }

    void initialiseLevels(){
        for (int i = 0; i<graph.nodes; i++){
            levels[i] = INT_MAX;
        }
    }

    void join(int *wl1, int size1, int *wl_final, int size2) {
        const size_t sizeof_wl1 = sizeof(wl1) * size1;
        std::memcpy(&wl_final[size2], wl1, sizeof_wl1);
    }

    void runBFS(){

        levels[src] = 0;
        WorkList *curr = new WorkList(graph.nodes);
        WorkList *next = new WorkList(graph.nodes);

        WorkList **threadwiseWL = new WorkList*[hclib::num_workers()];
        for (int t=0; t<hclib::num_workers(); t++){
            threadwiseWL[t] = new WorkList(graph.nodes);
        }

        int nextLevel = 0;

        next->add(src);

        while (!next->isEmpty()){
            std::swap(curr, next);
            next->reset();

            ++nextLevel;

            hclib::finish([=]() {
                hclib::loop_domain_t loop = {0, curr->size(), 1, std::max(1, int((curr->pos+1)/(FACTOR * hclib::num_workers())))};
                hclib::forasync1D(&loop, [=](int itr){
                int node = curr->nodesInWL[itr];

                for (int i = graph.nindex[node]; i<graph.nindex[node+1]; i++){
                    int neigh = graph.nlist[i];
                    if (levels[neigh] == INT_MAX){
                        levels[neigh] = nextLevel;
                        threadwiseWL[hclib::current_worker()]->add(neigh);
                    }
                }
            }, FORASYNC_MODE_RECURSIVE);
            });

            for(int t=0; t<hclib::num_workers(); t++){
                join(threadwiseWL[t]->nodesInWL, threadwiseWL[t]->size(),
                     next->nodesInWL, next->size());
                next->pos += threadwiseWL[t]->size();
                threadwiseWL[t]->reset();
            }

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

        printf("starting kernel.. \n");
        hclib::kernel([&]{ 
            runBFS();
        });
        printf("computations done \n");

        freeECLgraph(graph);
        delete[] levels;
      
        return 0;
    }
};


int main(int argc, char **argv)
{
    hclib::launch([&]{
        BFS bfs;
        bfs.main(argc, argv);
    });
    return 0;
}
