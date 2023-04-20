#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <cstring>
#include "ECLgraph.h"

int seed = 7;

struct Node_{
    int numEdge;
    bool alive;
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
            if (loopOptionKey == "-nbodies"){
                i += 1;
                nbodies = std::stoi(argv[i]);
            }
            if (loopOptionKey == "-k"){
                i += 1;
                k = std::stoi(argv[i]);
            }
            if (loopOptionKey == "-file"){
                i += 1;
                fileName = argv[i];
            }
            i += 1;
        }
    }

    void createInitialWorkList(WorkList *nextWL){
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
        createInitialWorkList(nextWL);

        while (nextWL->size() != 0){
            std::swap(currWL, nextWL);
            nextWL->reset();

            for (int i=0; i<currWL->size(); i++){
                int node = currWL->deadNodes[i];
                for (int neigh=graph.nindex[node]; neigh<graph.nindex[node+1]; neigh++){
                    int neighbour = graph.nlist[neigh];
                    if (nodes[neighbour].alive){
                            nodes[neighbour].numEdge--;
                
                        // check and update if this neighbour is dead
                        if (nodes[neighbour].numEdge < k){
                            nextWL->add(neighbour);
                            nodes[neighbour].alive = false;
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

        std::cout << "Starting computations... \n";
        auto startTime = std::chrono::steady_clock::now();
        findKCore();
        auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = endTime - startTime;
		std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000<<  " ms \n";

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
    KCore k;
    k.main(argc, argv);
}
