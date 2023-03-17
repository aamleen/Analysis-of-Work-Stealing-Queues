#include <random>
#include <iostream>
#include <string>
#include <chrono>

int seed = 0;
struct WorkEdge;

struct Node{
    int id;
    int numEdges = 0;
    WorkEdge **neighbours;

    Node(int i): id(i) { }
};


struct WorkEdge{
    Node *src;
    Node *dest;
    bool invalid = false;

    WorkEdge(Node *a1, Node *a2) : src(a1), dest(a2) {}
};


struct KTruss{
    public:
        Node **nodes;
        WorkEdge **currValidEdges;
        WorkEdge **nextValidEdges;
        int nbodies;
        int k;
        int totalEdges;


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
            i += 1;
        }
        printf("\nConfigurations: number of nodes- %d,  k- %d \n", nbodies, k);
    }


    int RandomInput(int nbodies){
        std::mt19937 gen(seed);
		#if __cplusplus >= 201103L
		std::uniform_real_distribution<double> dist(0, 1);
		#else
		std::uniform_real<double> dist(0, 1);
		#endif

        // create the nodes
        bool *edgeTracker = new bool[nbodies*nbodies];
        for (int i=0;i<nbodies;i++){
            Node *node = new Node(i);
            nodes[i] = node;
        }

        // randomly generate the graph
        for (int i=0;i<nbodies;i++){
            int numOutEdges = dist(gen)*std::min(100, nbodies/5);            
            int count = 0;
            while(count<numOutEdges){
                int nodeId = dist(gen)*(nbodies-count);
                Node *neigh = nodes[nodeId];
                if (neigh->id != nodes[i]->id){
                    if (edgeTracker[neigh->id * nbodies + nodes[i]->id] == 0){
                        edgeTracker[neigh->id * nbodies + nodes[i]->id] = 1;
                        edgeTracker[nodes[i]->id * nbodies + neigh->id] = 1;
                        neigh->numEdges++;
                        nodes[i]->numEdges++;
                    }
                    count++;
                }
            }
        }
        int totalEdges = 0;
        // update the adjacency list for each node from randomly generated graph
        for (int i=0;i<nbodies;i++){
            nodes[i]->neighbours = new WorkEdge*[nbodies];
            for (int neigh=0;neigh<nbodies;neigh++){
                if (edgeTracker[i * nbodies + neigh] == 1){
                    WorkEdge *workEdge;
                    if (i < neigh){
                        workEdge = new WorkEdge(nodes[i], nodes[neigh]);
                    }
                    else{
                        workEdge = nodes[neigh]->neighbours[i];
                    }
                    
                    nodes[i]->neighbours[neigh] = workEdge;
                    totalEdges++;
                }
                else{
                    nodes[i]->neighbours[neigh] = NULL;
                }
            }

        }
        delete[] edgeTracker;
        return totalEdges/2;
    }


    void getWorkEdges(WorkEdge **workEdges){
        int pos=0;
        for (int i=0; i<nbodies-1; i++){
            for (int neigh=i+1; neigh < nbodies; neigh++){
                WorkEdge *we = nodes[i]->neighbours[neigh];
                if (we != NULL){
                    workEdges[pos++] = we;
                }
            }
        }
    }


    int numTriangles(WorkEdge *workEdge){
        Node *src = workEdge->src;
        Node *dest = workEdge->dest;
        int num_triangles = 0;

        for (int nid=0;nid<nbodies;nid++){
            if (nodes[nid] == src || nodes[nid] == dest){
                continue;
            } 

            if ((src->neighbours[nid] != NULL && !src->neighbours[nid]->invalid) &&
                (dest->neighbours[nid] != NULL && !dest->neighbours[nid]->invalid)){
                    num_triangles++;
                }
        }

        return num_triangles;
    }


    void cleanArray(WorkEdge **arr){
        for (int i=0; i<totalEdges; i++){
            arr[i] = NULL;
        }
    }


    WorkEdge **findKTruss(){
        int numEdges = totalEdges;
        int iter = 0;
        while (true){
            iter++;
            cleanArray(nextValidEdges);
            bool removed = false;
            int nve_pos = -1;

            for (int i=0; i<totalEdges ;i++){
                WorkEdge *e = currValidEdges[i];
                if (e == NULL){
                    continue;
                }
                //check if e is included in less than k-2 triangles
                if (numTriangles(e) < k-2){
                    e->invalid = true;
                    removed = true;
                }
                else{
                    nextValidEdges[++nve_pos] = e;
                }
            }

            if (!removed){
                break;
            }

            std::swap(currValidEdges, nextValidEdges);
            numEdges = nve_pos+1;
        }

        return currValidEdges;
        
    }


    void main(int argc, char** argv){
        nbodies = 1000;
        k = 20;
        parseArgs(argc, argv);
        nodes = new Node*[nbodies];

        totalEdges = RandomInput(nbodies);
        currValidEdges = new WorkEdge*[totalEdges];
        nextValidEdges = new WorkEdge*[totalEdges];

        getWorkEdges(currValidEdges);
        printf ("starting calculations... \n");
        auto startTime = std::chrono::steady_clock::now();

        // the edges that are included in ktruss are returned in currValidEdges
        findKTruss();
        auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = endTime - startTime;
		std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000 <<  " ms \n";

        int ktruss = 0;
        for (int i=0; i<totalEdges; i++){
            if (currValidEdges[i]!=NULL){
                ktruss+=1;
            }
        }
        printf("Total number of edges: %d \n", totalEdges);
        printf("Number of edges in k-truss: %d \n", ktruss);

       


        // clean-up
        for (int i=0;i<nbodies-1;i++){
            for (int n=i+1; n<nbodies; n++){
                if (nodes[i]->neighbours[n] != NULL)
                    delete[] nodes[i]->neighbours[n];
            }
            delete[] nodes[i]->neighbours;
            delete[] nodes[i];
        }
        
        delete[] nodes;
        delete[] currValidEdges;
        delete[] nextValidEdges;
    }
};

int main(int argc, char** argv){
    KTruss kt;
    kt.main(argc, argv);
}