#include <random>
#include <iostream>
#include <string>
#include <chrono>
#include <list>
#include <limits>
#include "hclib.hpp"

struct Edge;
struct Node;

int seed = 0;
int FACTOR = 1;

struct Node{
    int id;
    int numEdges;
    Edge **neighbours;
    int edgePos = -1;

    Node(int i): id(i) { };

    void addNeighbour(Edge *e){
        this->neighbours[++this->edgePos] = e;
    }
};


struct Edge{
    Node *src;
    Node *dest;
    int weight;
    bool isInMST;
    Edge(int w, Node *s, Node *d): weight(w), dest(d), src(s), isInMST(false) { };

    Node* getneighbour(Node *n){
        if (n == src){
            return dest;
        }
        if (n == dest){
            return src;
        }
        return NULL;
    }
};

struct Component{
    std::list <Node *> comp_nodes;
    Edge *minEdge=NULL;
    Component *c_dest; // the component that the minEdge connects to
    Node *leader;

    Component(Node *n){
        comp_nodes.push_back(n);
        leader = n;
    }

    Node* getLeader(){
        return leader;
    }

    int getLeaderID(){
        return leader->id;
    }

    void resetMinEdge(){
        minEdge = NULL;
    }

    void addMinEdge(Edge *e, Component *c){
        if (minEdge == NULL){
            minEdge = e;
            c_dest = c;
        }
        else if (minEdge->weight > e->weight){
            minEdge = e;
            c_dest = c;
        }
    }
};

struct ComponentList{
    Component **components;
    int compPos;

    ComponentList(int n){
        components = new Component*[n];
        compPos = -1;
    }

    void InsertComponent(Component *comp){
        this->components[++this->compPos] = comp;
    }

    void resetComponentList(){
        compPos = -1;
    }

    int getComponentListSize(){
        return compPos+1;
    }
};


struct UnionFind{
    Component **parents;
    int *rank;

    UnionFind(int n){
        parents = new Component*[n];
        rank = new int[n];
    }

    void initializeParents(int n, Component **c){
        for (int i=0; i<n; i++){
            parents[i] = c[i];
            rank[i] = 0;
        }
    }

    int find(int i){
        if (this->parents[i]->getLeaderID() == i){
            return i;
        }
        else{
            return find(this->parents[i]->getLeaderID());
        }
    }

    Component *findComponent(int i){
        if (this->parents[i]->getLeaderID() == i){
            return this->parents[i];
        }
        else{
            int x = this->parents[i]->getLeaderID();
            return findComponent(x);
        }
    }

    Component *Union(int x, int y) 
    { 
        Component *xroot = findComponent(x); 
        Component *yroot = findComponent(y);

        int xrank = rank[xroot->getLeaderID()];
        int yrank = rank[yroot->getLeaderID()];
     
        if (xrank < yrank){
            this->parents[xroot->getLeaderID()] = yroot; 
            yroot->comp_nodes.splice(yroot->comp_nodes.end(), xroot->comp_nodes);
            xroot->leader = yroot->leader;
            return yroot;
        }
        else if (xrank > yrank) {
            this->parents[yroot->getLeaderID()] = xroot; 
            xroot->comp_nodes.splice(xroot->comp_nodes.end(), yroot->comp_nodes);
            yroot->leader = xroot->leader;
            return xroot;
        }
        else { 
            this->parents[yroot->getLeaderID()] = xroot;
            this->rank[xroot->getLeaderID()]++;
            xroot->comp_nodes.splice(xroot->comp_nodes.end(), yroot->comp_nodes);
            yroot->leader = xroot->leader;
            return xroot;
        } 
    } 
};


struct Boruvka{
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
	    if (loopOptionKey == "-FACTOR"){
                i += 1;
                FACTOR = std::stoi(argv[i]);
            }
            i += 1;
        }
        printf("\nConfigurations: number of nodes- %d, FACTOR - %d \n", nbodies, FACTOR);
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
            int rand = (int)(dist(gen)*(nbodies/2));
            int numOutEdges = rand; 
            int count = 0;
            if (i == 0){
                numOutEdges = nbodies-1;
                for (int j=1;j<nbodies;j++){
                    edgeTracker[j] = 1;
                    nodes[j]->numEdges++;
                    nodes[i]->numEdges++;
                }
                continue;
            }
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
        for (int i=0; i<nbodies; i++){
            nodes[i]->neighbours = new Edge*[nodes[i]->numEdges];
        }

        for (int i=0;i<nbodies-1;i++){
            for (int neigh=i+1;neigh<nbodies;neigh++){
                if (edgeTracker[i * nbodies + neigh] == 1){
                    int weight = dist(gen)*(1000);
                    Edge *e = new Edge(weight ,nodes[i], nodes[neigh]);
                    nodes[i]->addNeighbour(e);
                    nodes[neigh]->addNeighbour(e);
                    totalEdges++;
                }
            }
        }
        delete[] edgeTracker;
        return totalEdges;
    }

    void initializeComponentList(ComponentList *next){
        for (int i=0; i<nbodies; i++){
            Component *c = new Component(nodes[i]);
            next->InsertComponent(c);
        }
    }


    void findMST(){
        ComponentList *curr = new ComponentList(nbodies);
        ComponentList *temp = new ComponentList(nbodies);
        ComponentList *next = new ComponentList(nbodies);
        UnionFind *uf = new UnionFind(nbodies);

        initializeComponentList(next);
         uf->initializeParents(nbodies, next->components);
        int c=0;

        while (true){
            std::swap(curr, next);
            temp->resetComponentList();
            next->resetComponentList();
            int num_comp = curr->getComponentListSize();

            hclib::finish([&]() {
                hclib::loop_domain_t loop = {0, num_comp, 1, std::max(1,
                                            int(num_comp/(FACTOR * hclib::num_workers())))};
                hclib::forasync1D(&loop, [&](int i){
                    Component *c_src = curr->components[i];
                    int comp_id = c_src->getLeaderID();
                    for(Node *comp_node : c_src->comp_nodes){
                        for (int n = 0; n < comp_node->numEdges; n++){
                            Edge *e = comp_node->neighbours[n];
                            Node *neigh = e->getneighbour(comp_node);
                            Component *c_dest = uf->findComponent(neigh->id);
                            if (!e->isInMST && comp_id != c_dest->getLeaderID()){
                                c_src->addMinEdge(e, c_dest);
                            }
                        }
                    }
                }, FORASYNC_MODE_RECURSIVE);
            });

            for (int i = 0; i < curr->getComponentListSize(); i++){
                Component *c = curr->components[i];
        
                if (c->minEdge == NULL){
                    continue;
                }

                if (!c->minEdge->isInMST){
                    int comp1 = c->getLeaderID();
                    int comp2 = c->c_dest->getLeaderID();
                    if (comp1 == comp2)
                        continue;

                    Component *new_c = uf->Union(comp2, comp1);
                    temp->InsertComponent(new_c);
                    c->minEdge->isInMST = true;
                    new_c->minEdge = NULL;
                    
                }
            }

            for (int i = 0; i < temp->getComponentListSize(); i++){
                if (temp->components[i]->comp_nodes.size() > 0){
                    next->InsertComponent(temp->components[i]);
                }
            }

            if (next->getComponentListSize() == 0){
                break;
            }
        } 

        delete[] curr->components;
        delete[] temp->components;
        delete[] next->components;
        delete[] curr;  
        delete[] temp;  
        delete[] next;  
        delete[] uf->parents;
        delete[] uf->rank;
        delete[] uf;      
    }


    void main(int argc, char** argv){
        nbodies = 100;
        parseArgs(argc, argv);
        nodes = new Node*[nbodies];
        totalEdges = RandomInput(nbodies);

        printf("Starting kernel.\n");
        hclib::kernel([&](){
            findMST();
        });
        printf("Computations done..\n");

        delete[] nodes;
    } 

};


int main(int argc, char** argv){
    hclib::launch([&](){
        Boruvka br;
        br.main(argc, argv);
    });
}
