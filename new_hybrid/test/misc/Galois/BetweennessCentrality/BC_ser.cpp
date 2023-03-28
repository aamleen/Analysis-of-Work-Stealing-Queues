#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <atomic>

int FACTOR = 1;
int seed = 0;

struct Node{
    int id;
    double BC = 0;
    std::atomic_int d;
    double delta = 0;
    std::atomic_int sigma;

    int numInEdges = 0;
    int numOutEdges = 0;
    Node **outNeighbours;

    Node **successors;
    std::atomic_int succ_pos;

    Node(int i): id(i), d(0), sigma(0), succ_pos(0) { }

    void succ_append(int pos, Node *n){
        successors[pos] = n;
    }

    int succ_size(){
        return succ_pos.load();
    }
};


struct WorkList {
    Node **phaseNodes;
    int pos = -1;

    WorkList(int n){
        phaseNodes = new Node*[n];
    }

    void append(Node *n){
        phaseNodes[++pos] = n;
    }

    void insert_at(int p, Node *n){
        phaseNodes[p] = n;
        if (pos < p)
            pos = p;
    }

    void reset(){
        pos = -1;
    }

    int size(){
        return pos+1;
    }

    Node *get(int k){
        return phaseNodes[k];
    }
};



struct BetweennessCentrality{
    public:
        Node **nodes;
        int nbodies;
        int totalEdges;


   void parseArgs(int argc, char** argv) {
        int i = 0;
        while (i < argc) {
            std::string loopOptionKey = argv[i];
            if (loopOptionKey == "-nbodies"){
                i += 1;
                nbodies = std::stoi(argv[i]);
            }
            i += 1;
        }
    }

    void swap(Node **randNodes, int ind1, int ind2){
        Node *node = randNodes[ind1];
        randNodes[ind1] = randNodes[ind2];
        randNodes[ind2] = node;
    }

    void RandomInput(int nbodies){
        std::mt19937 gen(seed);
		#if __cplusplus >= 201103L
		std::uniform_real_distribution<double> dist(0, 1);
		std::uniform_real_distribution<double> edgeCount(0.5, 1);
		#else
		std::uniform_real<double> dist(0, 1);
		#endif

        Node **randNodes = new Node*[nbodies];
        for (int i=0;i<nbodies;i++){
            Node *node = new Node(i);
            nodes[i] = node;
            randNodes[i] = node;
        }   
        int totalEdges = 0;
        for (int i=0;i<nbodies;i++){
            int numOutEdges = (int)(edgeCount(gen)*nbodies/2);
            nodes[i]->outNeighbours = new Node*[numOutEdges];
            nodes[i]->numOutEdges = numOutEdges;

            int count = 0;
            while(count<numOutEdges){
                int nodeId = (int)(dist(gen)*(nbodies-1-count));
                Node *neigh = randNodes[nodeId];

                if (neigh->id != nodes[i]->id){
                    totalEdges++;
                    nodes[i]->outNeighbours[count++] = neigh;
                    neigh->numInEdges += 1;
                    swap(randNodes, nodeId, nbodies-1-count);
                }
            }
        }
        printf("\nConfigurations: number of nodes- %d, number of edges- %d \n", nbodies, totalEdges);
        delete[] randNodes;
    }

    void InitializeNodes(){
        for (int n=0; n<nbodies; n++){
            Node *node = nodes[n];
            node->d = -1;
            node->sigma = 0;
            node->succ_pos = 0;
        }
    }

    void resetWL(WorkList** S){
        for (int i=0; i<nbodies; i++)
            S[i]->reset();
    }

    void betweennessCentrality(WorkList **S){
        int phase = 0;
        std::atomic_int count;
        count=0;
        
        for (int i=0; i<nbodies; i++){
                S[i] = new WorkList(nbodies);
            nodes[i]->successors = new Node*[nodes[i]->numOutEdges];
    
        }
                
        for (int i=0; i<nbodies; i++){            
            InitializeNodes();
            Node *node = nodes[i];
            node->sigma = 1;
            node->d = 0;
            phase = 0;
            resetWL(S);
            S[phase]->append(node);
            count = 1;

            while (count > 0){
                count = 0;
                
                    for (int j=0; j<S[phase]->size(); j++){        
                        Node *v = S[phase]->get(j);
                        if (v != NULL){   
                            
                            for (int k=0; k<v->numOutEdges; k++){
                                Node *w = v->outNeighbours[k];
                                int dw = w->d.load();
                                int init = -1;
                                std::atomic_compare_exchange_weak(&w->d, &init, phase+1);
                                if (dw == -1){
                                    int p = std::atomic_fetch_add(&count, 1);
                                    S[phase+1]->insert_at(p, w);
                                    dw = phase + 1;
                                }
                                if (dw == phase + 1){
                                    int p = std::atomic_fetch_add(&v->succ_pos, 1);
                                    v->succ_append(p, w);
                                    std::atomic_fetch_add(&w->sigma, v->sigma.load());
                                }
                            }
                        }    
                    }
                phase+=1;
            }
            phase-=1;
          
            for (int j=0; j<nbodies; j++){
                Node *t = nodes[j];
                t->delta = 0.0;
            }

            while(phase > 0){
                    for (int k=0; k<S[phase]->size(); k++){
                        Node *w = S[phase]->get(k);
                        double delta_sum_w = 0.0;
                        int sigma_w = w->sigma.load();
                        for (int m=0; m<w->succ_size(); m++){
                            Node *v = w->successors[m];
                            if (v){
                                int v_sigma = v->sigma.load();
                                delta_sum_w += ((double)sigma_w/(double)v_sigma * (1+v->delta));
                            }
                        }
                        w->delta = delta_sum_w;
                        w->BC += delta_sum_w;
                    }
                phase-=1;
            }
        }
    }

    void printGraph(){
        for (int i=0;i<nbodies; i++){
            printf("%d --> ", nodes[i]->id);
            for (int j=0; j<nodes[i]->numOutEdges; j++){
                printf("%d ", nodes[i]->outNeighbours[j]->id);
            }
            printf("\n");
        }
    }


    void main(int argc, char** argv){
        nbodies = 100;
        parseArgs(argc, argv);
        nodes = new Node*[nbodies];

        RandomInput(nbodies);
        WorkList **S = new WorkList*[nbodies]; 

        auto startTime = std::chrono::steady_clock::now();
        betweennessCentrality(S);
        auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = endTime - startTime;
		std::cout << "\nElapsed time = " << elapsed_seconds.count()*1000 <<  " ms \n";

        // clean-up
        for (int i=0;i<nbodies;i++){
            delete[] nodes[i]->outNeighbours;
            delete[] nodes[i];
            delete[] S[i]->phaseNodes;
        }
        delete[] nodes;
        delete[] S;
    }
};

int main(int argc, char** argv){
    BetweennessCentrality bc;
    bc.main(argc, argv);
}