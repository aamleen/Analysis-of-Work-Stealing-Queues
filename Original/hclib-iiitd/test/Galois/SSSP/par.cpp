#include <random>
#include <iostream>
#include <chrono>
#include <climits>
#include <atomic>
#include <vector>

#include "hclib.hpp"
#include "ECLgraph.h"

//int seed = 0;
int FACTOR = 1;



struct Node{
    std::atomic_int dist;
};

struct SSSP
{
    public:
    	ECLgraph graph;
        const char* fileName;
        int src;
        int dst;
        Node* distances;
        int* old_dist;

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
            }else if (loopOptionKey == "-d"){
                i += 1;
                dst = std::stoi(argv[i]);
            }else if (loopOptionKey == "-FACTOR"){
                i += 1;
                FACTOR = std::stoi(argv[i]);
            }

            i += 1;
        }
	printf("Source-%d, Dest-%d, FACTOR-%d \n", src, dst, FACTOR);
    }

    void initialiseDist(){
    	for (int i = 0; i<graph.nodes; i++){
    	    distances[i].dist = INT_MAX;
            old_dist[i] = INT_MAX;
    	}
    }

    void findSSSP(){

        distances[src].dist = 0;

        bool updated = false;

        do{
            updated = false;
            hclib::finish([&]() {
                hclib::loop_domain_t loop = {0, graph.nodes, 1, graph.nodes/(FACTOR * hclib::num_workers())};
                hclib::forasync1D(&loop, [&](int itr){
                
                int node = itr;
                int nodeDist = distances[node].dist;

                if (nodeDist < old_dist[node]){

                    old_dist[node] = nodeDist;
                    updated = true;

                    for (int i = graph.nindex[node]; i<graph.nindex[node+1]; i++){
                        const auto updatedDist = nodeDist + graph.eweight[i];
                        int neigh = graph.nlist[i];
                        for(int atom_val=distances[neigh].dist;
                            atom_val > updatedDist &&
                            !distances[neigh].dist.compare_exchange_weak(atom_val, updatedDist, std::memory_order_relaxed);
                        );
                    }
                }
                }, FORASYNC_MODE_RECURSIVE);
            });      
        }while (updated);
        std::cout<<"\n\nDistance found is " << distances[dst].dist <<"\n";
    }


    int main(int argc, char **argv)
    {
        src = 0;
        dst = 1;
        parseArgs(argc, argv);

        graph = readECLgraph(fileName);
	
//	for (int i = 0; i< 200; i++){
//		printf("%d\n", graph.eweight[i]);
//	}

        distances = new Node[graph.nodes];
        old_dist = new int[graph.nodes];

        printf("\nConfigurations: number of nodes- %d, source- %d, destination- %d \n", graph.nodes, src, dst);

        initialiseDist();

        printf("starting kernel \n");
        hclib::kernel([&]{
            findSSSP();
        });
        printf("computations done \n");
        
        freeECLgraph(graph);
        delete[] distances;
        delete[] old_dist;
      
        return 0;
    }
};


int main(int argc, char **argv)
{
    hclib::launch([&]{
        SSSP sssp;
        sssp.main(argc, argv);
    });
    return 0;
}
