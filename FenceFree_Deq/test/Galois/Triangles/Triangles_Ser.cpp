#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <bits/stdc++.h>

#include "ECLgraph.h"

int seed = 0;


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
        for (int workEdge=0; workEdge<graph.edges; workEdge++){
            totalTri += numTriangles(workEdge);
        }
        return totalTri;
    }

    void printGraph(int nodes){
        for (int i=0; i<nodes; i++){
            printf("\n%d --> ", i);
            for (int j=graph.nindex[i]; j<graph.nindex[i+1]; j++){
                printf("%d ", graph.nlist[j]);
            }
        }
        printf("\n\n");
    }

    void main(int argc, char** argv){
        parseArgs(argc, argv);
        graph = readECLgraph(fileName);
        printf("\n Nodes-%d,  Edges-%d, Average degree-%lf \n", graph.nodes, graph.edges, (double)graph.edges/(double)graph.nodes);

        // printGraph(100);
        src = new int[graph.edges];
        int pos = 0;
        for (int i=0; i<graph.nodes; i++){
            int numEdges = graph.nindex[i+1] - graph.nindex[i];
            for (int j=0; j<numEdges; j++){
                src[pos++] = i;
            }
        }

        auto startTime = std::chrono::steady_clock::now();
        int numTriangles = calculateNumTriangles();
        printf("Number of triangles: %d \n", numTriangles);

        auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = endTime - startTime;
		std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000 <<  " ms \n";

        delete[] src;
        freeECLgraph(graph);
    }
};

int main(int argc, char** argv){
    Triangles tr;
    tr.main(argc, argv);
}