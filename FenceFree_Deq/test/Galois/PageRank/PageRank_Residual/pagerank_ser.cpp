#include <random>
#include <iostream>
#include <string>
#include <chrono>

int seed = 0; 

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
        PageNode **randNodes;
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
            else if (loopOptionKey == "-num_iters"){
                i += 1;
                iterations = std::stod(argv[i]);
            }
            i += 1;
        }
        printf("\nConfigurations: number of nodes- %d, tolerance- %lf, "
                "teleportation parameter- %lf, number of iterations- %d \n\n", 
                nbodies, epsilon, ALPHA, iterations);
    }


    void RandomInput(){
        std::mt19937 gen(seed);
		#if __cplusplus >= 201103L
		std::uniform_real_distribution<double> dist(0, 1);
		#else
		std::uniform_real<double> dist(0, 1);
		#endif

        randNodes = new PageNode*[nbodies];
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
        int accum = 0;
        while (true){
            for (int i=0;i < nbodies; i++){
                delta[i] = 0;

                if (residual[i] > epsilon){
                    double oldResidual = residual[i];
                    residual[i] = 0.0;
                    nodes[i]->pageRank += oldResidual;

                    if (nodes[i]->outDegree > 0){
                        delta[i] = oldResidual * ALPHA/nodes[i]->outDegree;
                        accum+=1;
                    }
                }
            }

            for (int i=0; i<nbodies; i++){
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
            }

            iter++;

            if (iter >= iterations){
                std::cout << "ERROR: failed to converge in " << iterations << " iterations \n";
                return;
            }

            if (iter >= iterations || accum == 0){
                break;
            }
            accum = 0;  
        }

        double l1Norm = 0;
        for (int i=0;i<nbodies;i++){
            l1Norm+= (std::abs(nodes[i]->pageRank));
        }

        for (int i=0;i<nbodies;i++){
            nodes[i]->pageRank /= l1Norm;
        }
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

        auto startTime = std::chrono::steady_clock::now();
        calcResidualPageRank();
        auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = endTime - startTime;
		std::cout << "\n\n  Elapsed time parr = " << elapsed_seconds.count()*1000 <<  " ms \n\n";


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
    PageRank pr;
    pr.main(argc, argv);
    
}
