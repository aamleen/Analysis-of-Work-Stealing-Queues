#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <cstring>
#include <mutex>
#include <atomic>
#include "hclib.hpp"
#include "ECLgraph.h"

int seed = 7;
int THRESHOLD = 100;
int FACTOR = 1;

struct Node_{
    std::atomic_int numEdge;
    bool alive;
    // int padding[14];
    int get_numEdges(){
        return numEdge.load();
    }
};


struct WorkList{
    int pos = -1;
    int *deadNodes;

    WorkList(int num){
        deadNodes = new int[num];
    }

    int size(){
        return pos+1;
    }

    void add(int neigh){
        deadNodes[++pos] = neigh;
    }

    void reset(){
        pos = -1;
    }
};


struct KCore{
    public:
        ECLgraph graph;
        int nbodies;
        int k;
        Node_ *nodes;
        const char* fileName;

    
    void parseArgs(int argc, char** argv) {
        int i = 0;
        while (i < argc) {
            std::string loopOptionKey = argv[i];
            if (loopOptionKey == "-k"){
                i += 1;
                k = std::stoi(argv[i]);
            }
            if (loopOptionKey == "-file"){
                i += 1;
                fileName = argv[i];
            }
            if (loopOptionKey == "-FACTOR"){
                i += 1;
                FACTOR = std::stoi(argv[i]);
            }
            if (loopOptionKey == "-THRESHOLD"){
                i += 1;
                THRESHOLD = std::stoi(argv[i]);
            }
            i += 1;
        }
	printf("k- %d, FACTOR- %d \n", k, FACTOR);
    }
    

    void join(int *wl1, int size1, int *wl_final, int size2) {
        const size_t sizeof_wl1 = sizeof(*wl1) * size1;
        std::memcpy(&wl_final[size2], wl1, sizeof_wl1);
    }

    void createInitialWorkList(WorkList *nextWL,  WorkList **threadwiseWL){
        for (int i=0;i<nbodies;i++){
            if (!nodes[i].alive){
                nextWL->deadNodes[++nextWL->pos] = i;
            }
        }      
    }

    void initializeArrays(){
        for (int i=0;i<nbodies; i++){
            nodes[i].alive = false;
            int e = graph.nindex[i+1] - graph.nindex[i];
            nodes[i].numEdge = e;
            if (e >= k)
                nodes[i].alive = true;
        }
    }


    void findKCore(){
        WorkList *nextWL = new WorkList(nbodies);
        WorkList *currWL = new WorkList(nbodies);
        WorkList **threadwiseWL = new WorkList *[hclib::num_workers()];
        for (int t=0;t<hclib::num_workers(); t++){
            threadwiseWL[t] = new WorkList(nbodies);
        }

        createInitialWorkList(nextWL, threadwiseWL);

        while (nextWL->size() != 0){
            std::swap(currWL, nextWL);
            nextWL->reset();

            if (currWL->size() > THRESHOLD){
                hclib::finish([=]() {
                    hclib::loop_domain_t loop = {0, currWL->size(), 1, std::max(1, int(currWL->pos/(FACTOR * hclib::num_workers())))};
                    hclib::forasync1D(&loop, [=](int i){
            
                        int node = currWL->deadNodes[i];
                        for (int neigh=graph.nindex[node]; neigh<graph.nindex[node+1]; neigh++){
                            int neighbour = graph.nlist[neigh];
                            if (nodes[neighbour].alive){
                                    nodes[neighbour].numEdge.fetch_sub(1, std::memory_order_relaxed);

                                // check and update if this neighbour is dead
                                if (nodes[neighbour].get_numEdges() < k){
                                    threadwiseWL[hclib::current_worker()]->add(neighbour);
                                    nodes[neighbour].alive = false;
                                }
                                
                            }
                        }
                    }, FORASYNC_MODE_RECURSIVE);
                });  
        
                for(int t=0; t<hclib::num_workers(); t++){
                    join(threadwiseWL[t]->deadNodes, threadwiseWL[t]->size(),
                        nextWL->deadNodes, nextWL->size());
                    nextWL->pos += threadwiseWL[t]->size();
                    threadwiseWL[t]->reset();
                }   
            }
            else{
                for (int i=0; i<currWL->size(); i++){
                    int node = currWL->deadNodes[i];
                        for (int neigh=graph.nindex[node]; neigh<graph.nindex[node+1]; neigh++){
                            int neighbour = graph.nlist[neigh];
                            if (nodes[neighbour].alive){
                                    nodes[neighbour].numEdge--;

                                // check and update if this neighbour is dead
                                if (nodes[neighbour].get_numEdges() < k){
                                    nextWL->add(neighbour);
                                    nodes[neighbour].alive = false;
                                }
                                
                            }
                        }
                }
            }          
        }
        delete[] nextWL->deadNodes;
        delete[] currWL->deadNodes;
        delete[] nextWL;
        delete[] currWL;
    }


    void main(int argc, char** argv){
        k = 0;
        parseArgs(argc, argv);
        graph = readECLgraph(fileName);
        nbodies = graph.nodes;
        printf("\nConfigurations: number of nodes- %d, number of edges- %d,  K- %d \n", nbodies, graph.edges, k);

        nodes = new Node_[nbodies];
        initializeArrays();

        printf("Starting kernel... \n");
        hclib::kernel([&](){
            findKCore();
        });
        printf("Computation done... \n");
        
        int numKCore = 0;
        for (int i=0;i<nbodies;i++){
            if (nodes[i].alive){
                numKCore++;
            }
        }
        printf("Number of nodes in KCore: %d \n", numKCore);

        freeECLgraph(graph);
        delete[] nodes;
    }
};


int main(int argc, char** argv){
    hclib::launch([&](){
        KCore k;
        k.main(argc, argv);
    });
}
