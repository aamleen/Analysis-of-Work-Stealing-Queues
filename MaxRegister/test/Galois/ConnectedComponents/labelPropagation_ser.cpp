#include <random>
#include <iostream>
#include <chrono>
#include "ECLgraph.h"

int seed = 0;
typedef unsigned int complabel;


struct QueueNode{
    int nodeId;
    QueueNode *next = NULL;

    QueueNode(){};
};


struct LabelProp
{
    public:
        ECLgraph graph;
        const char* fileName;
        complabel* componentIDs;

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

    void initialiseComponentID(){
    	for (int i = 0; i<graph.nodes; i++){
    		componentIDs[i] = i;
    	}
    }

    int countConnectedComponents(){
        bool* cnt = new bool[graph.nodes];
        for (int i = 0; i<graph.nodes; i++){
        	cnt[i] = 0;
        }
        int count = 0;

        for (int i=0; i<graph.nodes; i++){
            complabel label = componentIDs[i];
            if (!cnt[label]){
                cnt[label] = true;
                count+=1;
            }
        }

        return count;
    }

    void findConnectedComponents(){
        bool update = 0;

        do{
            update = false;
            for (int i=0; i<graph.nodes; i++){
                int current_starting = graph.nindex[i];
                int current_end = graph.nindex[i+1];
                complabel min = componentIDs[i];

                for (int j = current_starting; j<current_end; j++){
                    if (componentIDs[graph.nlist[j]] < min){
                        min = componentIDs[graph.nlist[j]];
                    }
                }
                if (min < componentIDs[i]){
                    componentIDs[i] = min;
                    update = true;
                }
            }
        }while(update);
    }

    bool checkComponents(int node, bool *visited){
        QueueNode *head = new QueueNode();
        head->nodeId = node;
        QueueNode *last = head;
        complabel label = componentIDs[node];

        while (head != NULL){
            int curr_id = head->nodeId;
            visited[curr_id] = true;

            for (int i = graph.nindex[curr_id]; i<graph.nindex[curr_id+1]; i++){
                if (componentIDs[graph.nlist[i]]!=label){
                    std::cout<<"Differently labelled nodes are part of same component\n";
                    return false;
                }

                if (!visited[graph.nlist[i]]){
                    last->next = new QueueNode();
                    last->next->nodeId = graph.nlist[i];
                    last = last->next;
                }
            }
            QueueNode *prev = head;
            head = head->next;
            delete prev;
        }

        return true;
    }

    void verify(){
        bool* visited = new bool[graph.nodes];

        for (int i=0; i<graph.nodes; i++)
        	visited[i] = 0;

        for (int i = 0; i<graph.nodes; i++){
            if (!visited[i]){
                if (!checkComponents(i, visited)){
                    return;
                }
            }
        }

        std::cout << "Components identified successfully\n";
    }

    void main(int argc, char **argv)
    {
    	parseArgs(argc, argv);

    	graph = readECLgraph(fileName);
        printf("configuration: %d nodes and %d edges\n", graph.nodes, graph.edges);
        printf("average degree: %.2f edges per node\n", 1.0 * graph.edges / graph.nodes);
        
        componentIDs = new complabel[graph.nodes];
        initialiseComponentID();

        auto startTime = std::chrono::steady_clock::now();
        findConnectedComponents();
        auto endTime = std::chrono::steady_clock::now();
        
        int componentCount = countConnectedComponents();
        printf("num of connected components in graph: %d\n", componentCount);
        
        std::chrono::duration<double> elapsed_seconds = endTime - startTime;
        std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000 <<  " ms \n";
        // verify();
        freeECLgraph(graph);
         delete[] componentIDs;
    }
};

int main(int argc, char **argv)
{
    LabelProp lp;
    lp.main(argc, argv);
}
