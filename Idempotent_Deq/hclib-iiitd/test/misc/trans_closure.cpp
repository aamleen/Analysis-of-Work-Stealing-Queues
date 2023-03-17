// C++ program to print transitive closure of a graph
#include <bits/stdc++.h>
#include "hclib.hpp"

using namespace std;

class Graph {
	int V; // No. of vertices
	bool** tc; // To store transitive closure
	void DFSUtil(int u, int v);

public:
	int** adj; // array of adjacency lists
	
	Graph(int V); // Constructor

	// function to add an edge to graph
	void addEdge(int v, int w) { adj[v][w]=1; adj[w][v]=1; }

	// prints transitive closure matrix
	void transitiveClosure();
	void printGraph();
};

Graph::Graph(int V)
{
	this->V = V;
		
	adj = new int*[V];
	tc = new bool*[V];
	for (int i = 0; i < V; i++) {
		adj[i] = new int[V];
		memset(adj[i], 0, V * sizeof(int));
		tc[i] = new bool[V];
		memset(tc[i], false, V * sizeof(bool));
	}
}

// A recursive DFS traversal function that finds
// all reachable vertices for s.
void Graph::DFSUtil(int s, int v)
{
	// Mark reachability from s to t as true.
	if (s == v) {
		tc[s][v] = true;
	}
	else
		tc[s][v] = true;

	// Find all the vertices reachable through v
	
	for (int i = 0; i < V; ++i) {
		if (adj[v][i]==1 && (tc[s][i] == false)) {
			if (i == s) {
				tc[s][i] = 1;
			}
			else {
				HCLIB_FINISH{
                    hclib::async([=]() {
                        		DFSUtil(s, i);
				    });
			    }
            }
		}
	}
}

// The function to find transitive closure. It uses
// recursive DFSUtil()
void Graph::transitiveClosure()
{
	// Call the recursive helper function to print DFS
	// traversal starting from all vertices one by one
	for (int i = 0; i < V; i++)
		DFSUtil(i,
				i); // Every vertex is reachable from self.

	/* for (int i = 0; i < V; i++) {
		for (int j = 0; j < V; j++)
			cout << tc[i][j] << " ";
		cout << endl;
	} */
	printf("Done\n");
	/* for(int i = 0; i < V; i++)
    	delete[] adj[i];
	delete[] adj; */
}

void Graph::printGraph(){
	for(int i=0;i<V;i++){
		printf("Vertex %d --> ",i);
		for(int j=0;j<V;j++)
			printf("%d, ",adj[i][j]);
		printf("\n");
	}
}

// Driver code
int main(int argc, char ** argv)
{
    hclib::launch([=]() {
    printf("Launched hclib\n");
	int n=20;
	if(argc > 1) n = atoi(argv[1]);
      Graph g(n);
	  int e=(int)(n*log(n));
	  int i=0;
	  while(i<e){
		int n1 = rand()%(n);
		int n2 = rand()%(n);
		if(g.adj[n1][n2]==1)
			continue;
		g.addEdge(n1,n2);
		i+=2;
	  }
	//g.printGraph();
	printf("Starting Transitive Closure (%d)..\n",n);
	cout << "Transitive closure matrix is \n";
      hclib::kernel([&]() {
      g.transitiveClosure();
      });
  });
	return 0;
}