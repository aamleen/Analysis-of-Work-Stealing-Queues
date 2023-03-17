#include <random>
#include <iostream>
#include <chrono>
#include <climits>
#include <atomic>

#include "ECLgraph.h"

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
            }
            i += 1;
        }
    }

    void initialiseDist(){
        for (int i = 0; i<graph.nodes; i++){
            distances[i].dist = {INT_MAX};
            old_dist[i] = INT_MAX;
        }
    }

    void findSSSP(){

        distances[src].dist = {0};

        bool updated = false;

        do
        {
            updated = false;
            for (int itr=0; itr<graph.nodes; itr++){
                int node = itr;
                int nodeDist = distances[node].dist;

                if (nodeDist < old_dist[node]){

                    old_dist[node] = nodeDist;
                    updated = true;

                    for (int i = graph.nindex[node]; i<graph.nindex[node+1]; i++){
                        auto updatedDist = nodeDist + graph.eweight[i];
                        int neigh = graph.nlist[i];
                        if (updatedDist < distances[neigh].dist){
                            distances[neigh].dist = updatedDist;
                        }
                    }
                }
            }
        }while (updated);
        std::cout<<"\n\nDistance found is " << distances[dst].dist <<"\n";
    }


    int main(int argc, char **argv)
    {
        src = 0;
        dst = 1;
        parseArgs(argc, argv);

        graph = readECLgraph(fileName);


        distances = new Node[graph.nodes];
        old_dist = new int[graph.nodes];

        printf("\nConfigurations: number of nodes- %d, source- %d, destination- %d \n", graph.nodes, src, dst);

        initialiseDist();

        auto startTime = std::chrono::steady_clock::now();
        findSSSP();
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = endTime - startTime;
        
        std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000 <<  " ms \n";

        freeECLgraph(graph);
        delete[] distances;
        delete[] old_dist;
      
        return 0;
    }
};


int main(int argc, char **argv)
{
    SSSP sssp;
    sssp.main(argc, argv);
    return 0;
}
