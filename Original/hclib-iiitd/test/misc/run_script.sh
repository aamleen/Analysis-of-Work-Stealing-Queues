cd ../../
./clean.sh
./install.sh
# source /mnt/hdd2/home/aamleen2022/Original/hclib-iiitd/hclib-install/bin/hclib_setup_env.sh
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/Original/hclib-iiitd/hclib-install/bin/hclib_setup_env.sh
cd test/misc
make

cd ../Galois/BarnesHut
make clean
make
cd ../BetweennessCentrality
make clean
make
cd ../BFS
make clean
make
cd ../Boruvka
make clean
make
cd ../ConnectedComponents
make clean
make
cd ../IndependentSet
make clean
make
cd ../K-core
make clean
make
cd ../K-truss
make clean
make
cd ../PageRank/PageRank_Residual
make clean
make
cd ../PageRank_Topological
make clean
make
cd ../../PointsToAnalysis
make clean
make
cd ../SSSP
make clean
make
cd ../Triangles
make clean
make

# cd /mnt/hdd2/home/aamleen2022/Original/hclib-iiitd/test/misc