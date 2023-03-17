#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <bits/stdc++.h> 
// #include <boost/algorithm/string.hpp> 
#include <cstdlib>
#include "ECLgraph.h"


int seed = 0;
typedef unsigned char stattype;


struct IndependentSet{
    public:
        static const stattype in = ~0 - 1;
        static const stattype out = 0; 
        stattype *nstatus;
        ECLgraph graph;
        const char* fileName;


    void parseArgs(int argc, char** argv) {
        int i = 0;
        while (i < argc) {
            std::string loopOptionKey = argv[i];
            if (loopOptionKey == "-file"){
                i += 1;
                fileName = argv[i];
            }
            i += 1;
        }
    }


    /* hash function to generate random values */

    // source of hash function: https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
    static unsigned int hash(unsigned int val) {
        val = ((val >> 16) ^ val) * 0x45d9f3b;
        val = ((val >> 16) ^ val) * 0x45d9f3b;
        return (val >> 16) ^ val;
    }


    void initialize_nstatus(const float scaledavg, const float avg){
        for (int i = 0; i < graph.nodes; i++) {
            stattype val = in;
            const int degree = graph.nindex[i + 1] - graph.nindex[i];
            if (degree > 0) {
            const float x = degree - (hash(i) * 0.00000000023283064365386962890625f);
            const int res = roundf((scaledavg / (avg + x)));
            val = (res + res) | 1;
            }
            nstatus[i] = val;
        }
    }


    void findMaximalIS(){
        const float avg = (float)graph.edges / graph.nodes;
        const float scaledavg = ((in / 2) - 1) * avg;
        initialize_nstatus(scaledavg, avg);
        
        int missing, ms;
        do {
            missing = 0;
            for (int v = 0; v < graph.nodes; v++) {
                int nsv = nstatus[v];
                if (nsv & 1) {
                    int i = graph.nindex[v];
                    while (i < graph.nindex[v+1]){
                        int nsi = nstatus[graph.nlist[i]];
                        if ((nsv < nsi) || ((nsv == nsi) && (v < graph.nlist[i]))) break;
                        i++;
                    }
    
                    while ((i < graph.nindex[v + 1]) && ((nsv > nstatus[graph.nlist[i]]) || 
                            ((nsv == nstatus[graph.nlist[i]]) && (v > graph.nlist[i])))) {
                        i++;
                    }
                    if (i < graph.nindex[v + 1]) {
                    missing = 1;
                    } else {
                    for (int i = graph.nindex[v]; i < graph.nindex[v + 1]; i++) {
                        nstatus[graph.nlist[i]] = out;
                    }
                    nstatus[v] = in;
                    }
                }
            }
            ms = missing;
        } while (missing != 0);
    }


    void countMaximalIS(){
        int count = 0;
        for (int v = 0; v < graph.nodes; v++) {
            if (nstatus[v] == in) {
            count++;
            }
        }
        printf("num of elements in independent set: %d (%.1f%%)\n", count, 100.0 * count /graph.nodes);
    }


    void verifyMaximalIS(){
        for (int v = 0; v < graph.nodes; v++) {
            if ((nstatus[v] != in) && (nstatus[v] != out)) {fprintf(stderr, "ERROR: found unprocessed node in graph\n\n");  exit(-1);}
            if (nstatus[v] == in) {
                for (int i = graph.nindex[v]; i < graph.nindex[v + 1]; i++) {
                    if (nstatus[graph.nlist[i]] == in) {fprintf(stderr, "ERROR: found adjacent nodes in MIS\n\n");  exit(-1);}
                }
            } 
            else {
                int flag = 0;
                for (int i = graph.nindex[v]; i < graph.nindex[v + 1]; i++) {
                    if (nstatus[graph.nlist[i]] == in) {
                    flag = 1;
                    }
                }
                if (flag == 0) {fprintf(stderr, "ERROR: set is not maximal\n\n");  exit(-1);}
            }
        }
    }


    int main(int argc, char** argv){
        parseArgs(argc, argv);

        graph = readECLgraph(fileName);
        printf("configuration: %d nodes and %d edges\n", graph.nodes, graph.edges);
        printf("average degree: %.2f edges per node\n", 1.0 * graph.edges / graph.nodes);
        nstatus = new stattype [graph.nodes];

     

        printf("Finding Maximal Independent Set... \n");
        auto startTime = std::chrono::steady_clock::now();
        findMaximalIS();
        auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = endTime - startTime;
		
        countMaximalIS();
        std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000<<  " ms \n";
        verifyMaximalIS();

        freeECLgraph(graph);
        delete [] nstatus;
        return 0;

    } 

};

int main(int argc, char** argv){
    IndependentSet IS;
    IS.main(argc, argv);
}
