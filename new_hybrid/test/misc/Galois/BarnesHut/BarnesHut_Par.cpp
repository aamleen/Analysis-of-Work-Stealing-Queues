#include <random>
#include <iostream>
#include <string>
#include <mutex>
#include "hclib.hpp"
#include <chrono>

// TODO: fix `deque full, local execution` in RecurseForce method
int seed = 0;
int FACTOR = 1;

struct OctTreeNodeData { // the internal nodes are cells that summarize their children's properties
	double mass;
	double posx;
	double posy;
	double posz;

	/**
	 * Constructor that initializes the mass with zero and the position with the
	 * passed in values.
	 * 
	 * @param px
	 *          double value used to initialize the x coordinate
	 * @param py
	 *          double value used to initialize the y coordinate
	 * @param pz
	 *          double value used to initialize the z coordinate
	 */
	OctTreeNodeData(double px, double py, double pz) : mass(0.0), posx(px), posy(py), posz(pz) { }
	

	/**
	 * This method determines whether a tree node is a leaf.
	 * 
	 * @return a bool indicating whether the current object is a leaf node
	 */
	virtual bool isLeaf() {
		return false;
	}

	/**
	 * This method reads the x coordinate of this node.
	 * 
	 * @return a double value that represents the x coordinate
	 */
	double get_posx() {
		return posx;
	}

	/**
	 * This method reads the y coordinate of this node.
	 * 
	 * @return a double value that represents the y coordinate
	 */
	double get_posy() {
		return posy;
	}

	/**
	 * This method reads the z coordinate of this node.
	 * 
	 * @return a double value that represents the z coordinate
	 */
	double get_posz() {
		return posz;
	}
};

/**
 * This class defines objects to hold the node data in the leaf nodes (the
 * bodies) of the tree built by the Barnes Hut application
 * {@link barneshut.main.BarnessHut}.
 */
struct OctTreeLeafNodeData: OctTreeNodeData { // the tree leaves are the bodies
	double velx;
	double vely;
	double velz;
	double accx;
	double accy;
	double accz;
  int id;

	int nodesTraversed = 0;

	/**
	 * Constructor that initializes the mass, position, velocity, and acceleration
	 * with zeros.
	 */
	OctTreeLeafNodeData():OctTreeNodeData(0.0, 0.0, 0.0) {
		velx = 0.0;
		vely = 0.0;
		velz = 0.0;
		accx = 0.0;
		accy = 0.0;
		accz = 0.0;
	}

	/**
	 * This method determines whether a tree node is a leaf.
	 * 
	 * @return a bool indicating whether the current object is a leaf node
	 */
	bool isLeaf() override{
		return true;
	}

	/**
	 * This method sets the velocity to the passed in values.
	 * 
	 * @param x
	 *          double value used to initialize the x component of the velocity
	 * @param y
	 *          double value used to initialize the y component of the velocity
	 * @param z
	 *          double value used to initialize the z component of the velocity
	 */
	void setVelocity(double x, double y, double z) {
		velx = x;
		vely = y;
		velz = z;
	}

	/**
	 * This method determines whether the current node has the same position as
	 * the passed node.
	 * 
	 * @param o
	 *          the object (a node) whose position is compared to the current
	 *          node's position
	 * @return a bool indicating whether the position is identical
	 */
	// @Override
	// public bool equals(Object o) {
	// 	OctTreeLeafNodeData n = (OctTreeLeafNodeData) o;
	// 	return (this.posx == n.posx && this.posy == n.posy && this.posz == n.posz);
	// }
};


struct NodeBarnessHut {
	OctTreeNodeData *data;
	NodeBarnessHut *child0;
	NodeBarnessHut *child1;
	NodeBarnessHut *child2;
	NodeBarnessHut *child3;
	NodeBarnessHut *child4;
	NodeBarnessHut *child5;
	NodeBarnessHut *child6;
	NodeBarnessHut *child7;

	NodeBarnessHut(OctTreeNodeData* d) :data(d) { }

	OctTreeNodeData* getData() {
		return data;
	}

	NodeBarnessHut* getChild(int i) {
		switch (i) {
		case 0: return child0;
		case 1: return child1;
		case 2: return child2;
		case 3: return child3;
		case 4: return child4;
		case 5: return child5;
		case 6: return child6;
		case 7: return child7;
		}
		return NULL;
	}

	void setChild(int i, NodeBarnessHut *n) {
		switch (i) {
		case 0: child0 = n; break;
		case 1: child1 = n; break;
		case 2: child2 = n; break;
		case 3: child3 = n; break;
		case 4: child4 = n; break;
		case 5: child5 = n; break;
		case 6: child6 = n; break;
		case 7: child7 = n; break;
		}
	}
};

/**
 * BarnessHut class of the Barnes Hut application.
 */
struct BarnessHut {
	public:
		int nbodies; // number of bodies in system
		int ntimesteps; // number of time steps to run
		double dtime; // length of one time step
		double eps; // potential softening parameter
		double tol; // tolerance for stopping recursion, should be less than 0.57 for 3D case to bound error

		double dthf, epssq, itolsq;
		OctTreeLeafNodeData **body; // the n bodies
		double diameter, centerx, centery, centerz;
		int curr;
		std::mutex *mutex_locks;


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
        printf("\nConfigurations: number of nodes- %d, FACTOR- %d \n", nbodies, FACTOR);
    }


	void RandomInput() {
		std::mt19937 gen(seed);
		#if __cplusplus >= 201103L
		std::uniform_real_distribution<double> dist(0, 1);
		#else
		std::uniform_real<double> dist(0, 1);
		#endif

		double vx, vy, vz;
		ntimesteps = 1;
		dtime = 0.025;
		eps = 0.05;
		tol = 0.5;
		dthf = 0.5 * dtime;
		epssq = eps * eps;
		itolsq = 1.0 / (tol * tol);
	
		body = new OctTreeLeafNodeData*[nbodies];
		for (int i = 0; i < nbodies; i++) {
			body[i] = new OctTreeLeafNodeData();
      		body[i]->id = i;
			body[i]->mass = 1.0E-04;
			body[i]->posx = dist(gen);
			body[i]->posy = dist(gen);
			body[i]->posz = dist(gen);
			vx = dist(gen);
			vy = dist(gen);
			vz = dist(gen);
			(*body[i]).setVelocity(vx, vy, vz);
		}
	}

	/*
	 * Computes a bounding box around all the bodies.
	 */
	void ComputeCenterAndDiameter() {
		double minx, miny, minz;
		double maxx, maxy, maxz;
		double posx, posy, posz;
		minx = miny = minz = std::numeric_limits<double>::max();
		maxx = maxy = maxz = std::numeric_limits<double>::min();
		for (int i = 0; i < nbodies; i++) {
			posx = body[i]->posx;
			posy = body[i]->posy;
			posz = body[i]->posz;

			if (minx > posx) {
				minx = posx;
			}
			if (miny > posy) {
				miny = posy;
			}
			if (minz > posz) {
				minz = posz;
			}
			if (maxx < posx) {
				maxx = posx;
			}
			if (maxy < posy) {
				maxy = posy;
			}
			if (maxz < posz) {
				maxz = posz;
			}
		}
		diameter = maxx - minx;
		if (diameter < (maxy - miny)) {
			diameter = (maxy - miny);
		}
		if (diameter < (maxz - minz)) {
			diameter = (maxz - minz);
		}
		centerx = (maxx + minx) * 0.5;
		centery = (maxy + miny) * 0.5;
		centerz = (maxz + minz) * 0.5;		
	}

	/*
	 * Recursively inserts a body into the octree.
	 */
	void Insert(NodeBarnessHut *root, OctTreeLeafNodeData *b,
			double r) {
		double x = 0.0, y = 0.0, z = 0.0;
		OctTreeNodeData *n = (*root).getData();

		int i = 0;
		if (n->posx < b->posx) {
			i = 1;
			x = r;
		}
		if (n->posy < b->posy) {
			i += 2;
			y = r;
		}
		if (n->posz < b->posz) {
			i += 4;
			z = r;
		}
						
		NodeBarnessHut *child = (*root).getChild(i);
		if (child==NULL) {
			NodeBarnessHut *newnode = new NodeBarnessHut((OctTreeNodeData*)b);;
			(*root).setChild(i, newnode);
		} 
		else {
			double rh = 0.5 * r;
			OctTreeNodeData *ch = (*child).getData();
			if (!((*ch).isLeaf())) {
				Insert(child, b, rh);
			} else {
				OctTreeNodeData *oct = new OctTreeNodeData(n->posx - rh + x, n->posy - rh + y, n->posz - rh + z);
				NodeBarnessHut *newnode = new NodeBarnessHut(oct);;
				Insert(newnode, b, rh);
				Insert(newnode, (OctTreeLeafNodeData *) ch, rh);
				(*root).setChild(i, newnode);
			}
		}
	}

	/*
	 * Traverses the tree bottom up to compute the total mass and the center of
	 * mass of all the bodies in the subtree rooted in each internal octree node.
	 */
	void ComputeCenterOfMass(NodeBarnessHut *root) {
		double m, px = 0.0, py = 0.0, pz = 0.0;
		OctTreeNodeData *n = (*root).getData();
		
		int j = 0;
		n->mass = 0.0;
		for (int i = 0; i < 8; i++) {
			NodeBarnessHut *child = (*root).getChild(i);
			if (child!=NULL) {
				if (i != j) {
					(*root).setChild(i, NULL);
					(*root).setChild(j, child);
				}
				j++;
				OctTreeNodeData *ch = (*child).getData();
				if ((*ch).isLeaf()) {
					body[curr++] = (OctTreeLeafNodeData *) ch;
				} 
				else {
					ComputeCenterOfMass(child);
				}
				m = ch->mass;
				n->mass += m;
				px += ch->posx * m;
				py += ch->posy * m;
				pz += ch->posz * m;
			}
		}
		
		m = 1.0 / n->mass;
		n->posx = px * m;
		n->posy = py * m;
		n->posz = pz * m;
	}


	void RecurseForce(OctTreeLeafNodeData *nd, NodeBarnessHut *nn, double dsq, double epssq) {
		
		if (nn == NULL) {
			return;
		}
		double drx, dry, drz, drsq, nphi, scale, idr;
		OctTreeNodeData *n = (*nn).getData();
		drx = n->posx - nd->posx;
		dry = n->posy - nd->posy;
		drz = n->posz - nd->posz;
		drsq = drx * drx + dry * dry + drz * drz;

		// mutex_locks[nd->id].lock();
		 	nd->nodesTraversed++;
		// mutex_locks[nd->id].unlock();
		if (drsq < dsq) {
			if (!((*n).isLeaf())) { // n is a cell
				/*
				hclib::finish([=, &nn, &nd]() {
					hclib::loop_domain_t loop = {0, 8, 1, 1};
					hclib::forasync1D(&loop, [=, &nn, &nd](int i){	
								
							RecurseForce(nd, (*nn).getChild(i), dsq * 0.25, epssq);
					
					}, FORASYNC_MODE_FLAT);
				});
				*/
				for (int i=0;i<8;i++){	
					RecurseForce(nd, (*nn).getChild(i), dsq * 0.25, epssq);
				}
			}
			else { // n is a body
				if (n != nd) {
					drsq += epssq;
					idr = 1 / std::sqrt(drsq);
					nphi = n->mass * idr;
					scale = nphi * idr * idr;
					// mutex_locks[nd->id].lock();
						nd->accx += drx * scale;
						nd->accy += dry * scale;
						nd->accz += drz * scale;
					// mutex_locks[nd->id].unlock();
				}
			}
		} else { // node is far enough away, don't recurse any deeper
			drsq += epssq;
			idr = 1 / std::sqrt(drsq);
			nphi = n->mass * idr;
			scale = nphi * idr * idr;
			// mutex_locks[nd->id].lock();
				nd->accx += drx * scale;
				nd->accy += dry * scale;
				nd->accz += drz * scale;
			// mutex_locks[nd->id].unlock();
		}
	}

	/*
	 * Calculates the force acting on a body
	 */
	void ComputeForce(OctTreeLeafNodeData *nd, NodeBarnessHut *root, double size, double itolsq, int step, double dthf, double epssq) {
		double ax, ay, az;
		ax = nd->accx;
		ay = nd->accy;
		az = nd->accz;
		nd->accx = 0.0;
		nd->accy = 0.0;
		nd->accz = 0.0;
		RecurseForce(nd, root, size * size * itolsq, epssq);
		
		if (step > 0) {
			nd->velx += (nd->accx - ax) * dthf;
			nd->vely += (nd->accy - ay) * dthf;
			nd->velz += (nd->accz - az) * dthf;
		}
	} 
	/*
	 * Advances a body's velocity and position by one time step
	 */
	void Advance(double dthf, double dtime) {
		double dvelx, dvely, dvelz;
		double velhx, velhy, velhz;

		for (int i = 0; i < nbodies; i++) {
			OctTreeLeafNodeData *nd = body[i];
			body[i] = nd;
			dvelx = nd->accx * dthf;
			dvely = nd->accy * dthf;
			dvelz = nd->accz * dthf;
			velhx = nd->velx + dvelx;
			velhy = nd->vely + dvely;
			velhz = nd->velz + dvelz;
			nd->posx += velhx * dtime;
			nd->posy += velhy * dtime;
			nd->posz += velhz * dtime;
			nd->velx = velhx + dvelx;
			nd->vely = velhy + dvely;
			nd->velz = velhz + dvelz;
		}
	}


	void deleteTree(NodeBarnessHut *root){
		if (root == NULL){
			return;
		}
		//delete children
		deleteTree(root->child0);
		deleteTree(root->child1);
		deleteTree(root->child2);
		deleteTree(root->child3);
		deleteTree(root->child4);
		deleteTree(root->child5);
		deleteTree(root->child6);
		deleteTree(root->child7);

		//delete root
		if (root->data != NULL){
			delete[] root->data;
		}
		delete[] root;
	}

	void computeBarnesHut(OctTreeNodeData *oct, NodeBarnessHut *root, int step){
		double radius = diameter * 0.5;
		for (int i = 0; i < nbodies; i++) {
			Insert(root, body[i], radius); // grow the tree by inserting each body
		}
		curr = 0;
		// summarize subtree info in each internal node (plus restructure tree and sort bodies for performance reasons)
		ComputeCenterOfMass(root);
		ComputeForceAll(root, step);
		Advance(dthf, dtime); // advance the position and velocity of each body
	}


	void main(int argc, char** argv) {
		nbodies = 100000;
		parseArgs(argc, argv);
		RandomInput();
		mutex_locks = new std::mutex[nbodies];
		OctTreeNodeData *res;
		OctTreeNodeData *oct;
		NodeBarnessHut *root; 
		int step = 0;
		int s = step;

		printf("Launching hclib::kernel\n");
		hclib::kernel([&](){
			ComputeCenterAndDiameter();
			oct = new OctTreeNodeData(centerx, centery, centerz);
			root =  new NodeBarnessHut(oct);
			computeBarnesHut(oct, root, step);
		});
		printf("Finished computations\n");
		
		res = (*root).getData();

		long int totalNodesTraversed = 0;
		for (int i = 0; i < nbodies; i++) {
			totalNodesTraversed += body[i]->nodesTraversed;
		}	

		std::cout << totalNodesTraversed << " nodes traversed\n";	

		// clean-up
		deleteTree(root);
		delete[] body;	
		delete[] mutex_locks;	
	}
	

	void ComputeForceAll(NodeBarnessHut *root, int s) {	
		hclib::finish([=, &root]() {
			hclib::loop_domain_t loop = {0, nbodies, 1, int(nbodies/(FACTOR * hclib::num_workers()))};
            hclib::forasync1D(&loop, [=, &root](int i){		
					ComputeForce(body[i], root, diameter, itolsq, s, dthf, epssq);
		 	}, FORASYNC_MODE_RECURSIVE);
		});
	}
};

int main(int argc, char** argv){
	hclib::launch([&](){
		BarnessHut bs;
		bs.main(argc, argv);
	});
	return 0;
}
