#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include "hclib.hpp"


int seed = 0; 
int FACTOR = 1;


struct PageNode{
    int id;
    int outDegree = 0;
    int inDegree;
    PageNode **inEdges;
    double pageRank = 0.0;

    PageNode(int i): id(i) { }

};


struct PageRank{
    public:
        PageNode **nodes;
        int nbodies;
        double *delta;
        double *residual;
        double epsilon;
        double ALPHA;
        double INIT_RESIDUAL;
        int iterations;

    void swap(PageNode **randNodes, int ind1, int ind2){
        PageNode *node = randNodes[ind1];
        randNodes[ind1] = randNodes[ind2];
        randNodes[ind2] = node;
    }


        void parseArgs(int argc, char** argv) {
        int i = 0;
        while (i < argc) {
            std::string loopOptionKey = argv[i];
            if (loopOptionKey == "-nbodies"){
                i += 1;
                nbodies = std::stoi(argv[i]);
            }
            else if (loopOptionKey == "-epsilon"){
                i += 1;
                epsilon = std::stod(argv[i]);
            }
            else if (loopOptionKey == "-alpha"){
                i += 1;
                ALPHA = std::stod(argv[i]);
            }
            else if (loopOptionKey == "-FACTOR"){
                i += 1;
                FACTOR = std::stoi(argv[i]);
            }
            else if (loopOptionKey == "-num_iters"){
                i += 1;
                iterations = std::stod(argv[i]);
            }

            i += 1;
        }
        printf("\nConfigurations: number of nodes- %d, tolerance- %lf,"
         "teleportation parameter- %lf, number of iterations= %d, FACTOR- %d \n\n", 
         nbodies, epsilon, ALPHA,iterations, FACTOR);
    }


    void RandomInput(){
        std::mt19937 gen(seed);
		#if __cplusplus >= 201103L
		std::uniform_real_distribution<double> dist(0, 1);
		#else
		std::uniform_real<double> dist(0, 1);
		#endif

        PageNode **randNodes = new PageNode*[nbodies];
        for (int i=0;i<nbodies;i++){
            PageNode *node = new PageNode(i);
            nodes[i] = node;
            randNodes[i] = node;
        }

        for (int i=0;i<nbodies;i++){
            int numInEdges = dist(gen)*nbodies/2;
            nodes[i]->inEdges = new PageNode*[numInEdges];
            nodes[i]->inDegree = numInEdges;
            
            int count = 0;
            while(count<numInEdges){
                int nodeId = dist(gen)*(nbodies-count);
                PageNode *neigh = randNodes[nodeId];
                if (neigh->id != nodes[i]->id){
                    nodes[i]->inEdges[count] = neigh;
                    neigh->outDegree+=1;
                    count++;
                    swap(randNodes, nodeId, nbodies-1-count);
                }
            }
        }
        delete[] randNodes;
    }

    void initResidual(){
        for (int i=0;i<nbodies;i++){
            residual[i] = INIT_RESIDUAL;
            delta[i] = 0.0;
        }
    }

    void calcResidualPageRank(){
        int iter = 0;
        int PADDING = 16;
        int *accum = new int[hclib::num_workers()*PADDING];
        
        while (true){
            hclib::finish([&]() {
                hclib::loop_domain_t loop = {0, nbodies, 1, int(nbodies/(FACTOR * hclib::num_workers()))};
                hclib::forasync1D(&loop, [&](int i){
                    delta[i] = 0;

                    if (residual[i] > epsilon){
                        double oldResidual = residual[i];
                        residual[i] = 0.0;
                        nodes[i]->pageRank += oldResidual;

                        if (nodes[i]->outDegree > 0){
                            delta[i] = oldResidual * ALPHA/nodes[i]->outDegree;
                            accum[hclib::current_worker() * PADDING] += 1;
                        }
                    }
                }, FORASYNC_MODE_RECURSIVE);
            });

            hclib::finish([&]() {
                hclib::loop_domain_t loop = {0, nbodies, 1, int(nbodies/(FACTOR * hclib::num_workers()))};
                hclib::forasync1D(&loop, [&](int i){
                    double sum = 0;
                    for (int n=0; n< nodes[i]->inDegree; n++){
                        PageNode *neigh = nodes[i]->inEdges[n];
                        
                        if (delta[neigh->id] > 0){
                            sum += delta[neigh->id];
                        }
                    }
                    if (sum > 0){
                        residual[i] = sum;
                    }
                }, FORASYNC_MODE_RECURSIVE);
            });

            iter++;
            int totalAccum=0;
            for (int a=0; a< hclib::num_workers(); a++){
                totalAccum += accum[a*PADDING];
                accum[a*PADDING] = 0;
            }

            if (iter >= iterations){
                std::cout << "ERROR: failed to converge in " << iterations << " iterations \n";
            }
            if (iter >= iterations || totalAccum == 0){
                break;
            }
            totalAccum = 0;  
        }

        double l1Norm = 0;
        for (int i=0;i<nbodies;i++){
            l1Norm+= (std::abs(nodes[i]->pageRank));
        }

        hclib::finish([&]() {
            hclib::loop_domain_t loop = {0, nbodies, 1, int(nbodies/(FACTOR * hclib::num_workers()))};
            hclib::forasync1D(&loop, [&](int i){		
                nodes[i]->pageRank /= l1Norm;
            }, FORASYNC_MODE_RECURSIVE);
        });
    }

    void main(int argc, char** argv){
        nbodies = 10000;
        iterations = 1000;
        epsilon = 1.0e-3;
        ALPHA = 0.85;
        INIT_RESIDUAL = 1 - ALPHA;

        parseArgs(argc, argv);
        nodes = new PageNode*[nbodies];
        delta = new double[nbodies];
        residual = new double[nbodies];
        RandomInput();
        initResidual();

        printf("Kernel starting \n");
         hclib::kernel([&]{
            calcResidualPageRank();
         });
        printf("Computations completed. Kernel terminating \n");
     
        for(int i=0;i<nbodies;i++){
            delete[] nodes[i]->inEdges;
            delete[] nodes[i];
        }
        delete[] nodes;
        delete[] residual;
        delete[] delta;
    }
};


int main(int argc, char** argv){
    hclib::launch([&]{
        PageRank pr;
        pr.main(argc, argv);
    });   
}
