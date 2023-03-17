#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include "hclib.hpp"

#include "ECLgraph.h"

#define PADDING 16
int seed = 0;
int FACTOR = 1;

struct Triangles{
    public:
        int *src;
        ECLgraph graph;
        const char* fileName;


   void parseArgs(int argc, char** argv) {
        int i = 0;
        while (i < argc) {
            std::string loopOptionKey = argv[i];
            if (loopOptionKey == "-file"){
                i+=1;
                fileName = argv[i];
            }
            if (loopOptionKey == "-FACTOR"){
                i+=1;
                FACTOR = std::stoi(argv[i]);
            }

            i += 1;
        }
    }


    int numTriangles(int edge){
        int srcNode = src[edge];
        int destNode = graph.nlist[edge];
        int numSrcNeigh = graph.nindex[srcNode+1];
        int numDestNeigh = graph.nindex[destNode+1];
        int srcNeighPtr = graph.nindex[srcNode];
        int destNeighPtr = graph.nindex[destNode];
    
        int numTri = 0;
        while (srcNeighPtr < numSrcNeigh && destNeighPtr < numDestNeigh){
            if (graph.nlist[srcNeighPtr] == graph.nlist[destNeighPtr]){
                if (srcNode < graph.nlist[srcNeighPtr] && 
                        destNode > graph.nlist[srcNeighPtr]){
                    numTri++;
                }
                srcNeighPtr++;
                destNeighPtr++;
            }
            else if (graph.nlist[srcNeighPtr] < graph.nlist[destNeighPtr]){
                srcNeighPtr++;
            }
            else if (graph.nlist[srcNeighPtr] > graph.nlist[destNeighPtr]){
                destNeighPtr++;
            }
        }
        return numTri;

    }

     int calculateNumTriangles(){
        int totalTri = 0;
        int *countTri = new int[hclib::num_workers()*PADDING];
        for (int i=0; i<hclib::num_workers(); i++){
            countTri[i*PADDING] = 0;
        }
        hclib::finish([&]() {
            hclib::loop_domain_t loop = {0, graph.edges, 1, int(graph.edges/(FACTOR * hclib::num_workers()))};
            hclib::forasync1D(&loop, [&](int workEdge){
                int num = numTriangles(workEdge);
                countTri[hclib::current_worker()*PADDING] += num;
                
            }, FORASYNC_MODE_RECURSIVE);
        });
        for (int i=0;i<hclib::num_workers();i++){
            totalTri+=countTri[i*PADDING];
        }

        delete[] countTri;
        return totalTri;
    }


    void main(int argc, char** argv){
        parseArgs(argc, argv);
        
        graph = readECLgraph(fileName);
        printf("\n Nodes-%d,  Edges-%d, Average degree-%lf \n", graph.nodes, graph.edges, (double)graph.edges/(double)graph.nodes);

        src = new int[graph.edges];
        int pos = 0;
        for (int i=0; i<graph.nodes; i++){
            int numEdges = graph.nindex[i+1] - graph.nindex[i];
            for (int j=0; j<numEdges; j++){
                src[pos++] = i;
            }
        }

        hclib::kernel([&]{
            int numTriangles = calculateNumTriangles();
            printf("Number of triangles: %d \n", numTriangles);
        });

        delete[] src;
        freeECLgraph(graph);
    }
};

int main(int argc, char** argv){
    hclib::launch([&]{
        Triangles tr;
        tr.main(argc, argv);
    });
}



   