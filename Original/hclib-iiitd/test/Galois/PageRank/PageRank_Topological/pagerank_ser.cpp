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
    double pagerank;

    PageNode(int i, double rank): id(i), pagerank(rank) { }

};


struct PageRank{
    public:
        PageNode **nodes;
        int nbodies;
        double *newRanks;
        double epsilon;
        double ALPHA;
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
        printf("\nConfigurations: number of nodes- %d, tolerance- %lf,"
                "teleportation parameter- %lf, number of iterations= %d \n\n", 
                nbodies, epsilon, ALPHA, iterations);
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
            PageNode *node = new PageNode(i, 1-ALPHA);
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

    void calculateNewRank(PageNode *node, int ind){
        double rank=0.0;
        int numOutEdges =  node->outDegree;

        for (int i=0; i < node->inDegree; i++){
            PageNode *neigh = node->inEdges[i];
            double temp = neigh->pagerank/neigh->outDegree;
            rank+=temp;
        }
        rank = rank*ALPHA + (1-ALPHA);
        newRanks[ind] = rank;
    }

    bool updateNewRanks(){
        bool withinTolerance = true;
        for(int i=0;i<nbodies;i++){
            if (std::abs(nodes[i]->pagerank - newRanks[i]) > epsilon) {
                if (withinTolerance){
                    withinTolerance = false;
                }
            }
            nodes[i]->pagerank = newRanks[i];
        }
        return withinTolerance;
    }

    void calculatePageRank(){
        int iter = 0;
        while (true){
            for (int i=0;i<nbodies;i++){
                PageNode *node = nodes[i];
                calculateNewRank(node, i);
            }
            bool withinTolerance = updateNewRanks();
            if (withinTolerance){
                break;
            }
            iter++;
            if (iter >= iterations){
                std::cout << "Didn't converge in " << iterations << " iterations \n";
                return;
            }  
        }

        double l1Norm = 0;
        for (int i=0;i<nbodies;i++){
            l1Norm+= (std::abs(nodes[i]->pagerank));
        }

        for (int i=0;i<nbodies;i++){
            nodes[i]->pagerank /= l1Norm;
        }
    }

    void main(int argc, char** argv){
        nbodies = 10000;
        iterations = 1000;
        epsilon = 1.0e-3;
        ALPHA = 0.85;

        parseArgs(argc, argv);
        nodes = new PageNode*[nbodies];
        newRanks = new double[nbodies];
        RandomInput();

        auto startTime = std::chrono::steady_clock::now();
        calculatePageRank();
        auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = endTime - startTime;
		std::cout << "\n\n  Elapsed time parr = " << elapsed_seconds.count()*1000 <<  " ms \n\n";


        for(int i=0;i<nbodies;i++){
            delete[] nodes[i]->inEdges;
            delete[] nodes[i];
        }
        delete[] nodes;
        delete[] newRanks;
    }
};


int main(int argc, char** argv){
    PageRank pr;
    pr.main(argc, argv);
    
}
