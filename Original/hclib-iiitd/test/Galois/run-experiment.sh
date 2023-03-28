#!/bin/bash

# List of Different benchmarks that you have to experiment
BENCHMARKS=( "/BarnesHut/barnes_par" \
            "/BetweennessCentrality/bc_par" \
            "/BFS/bfs_par" \
            "/Boruvka/boruvka_par" \
            "/ConnectedComponents/labelPropagation_par" \
            "/IndependentSet/independentset_par" \
            "/K-core/kcore_par" \
            "/K-truss/ktruss_par" \
            "/PageRank/PageRank_Residual/pagerank_par" \
            "/PageRank/PageRank_Topological/pagerank_par" \
            "/PointsToAnalysis/pta_par" \
            "/SSSP/sssp_par" \
            "/Triangles/triangle_par" \
         )

NAMES=( "BarnesHut" \
        "BetweennessCentrality" \
        "BFS" \
        "Boruvka" \
        "ConnectedComponents" \
        "IndependentSet" \
        "K-core" \
        "K-truss" \
        "PageRank_Residual" \
        "PageRank_Topological" \
        "PointsToAnalysis" \
        "SSSP" \
        "Triangles" \
        )

GRAPH_PATH="/home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/Original/hclib-iiitd/test/Galois/graph"

ARGS=( "-nbodies 1000000 -FACTOR 32" \
        "-nbodies 1500 -FACTOR 4" \
        "-file ${GRAPH_PATH}/amazon0601.egr -FACTOR 48" \
        "-nbodies 5000 -FACTOR 32" \
        "-file ${GRAPH_PATH}/uk-2002.egr -FACTOR 32" \
        "-file ${GRAPH_PATH}/uk-2002.egr -FACTOR 8" \
        "-file ${GRAPH_PATH}/amazon0601.egr -k 65 -FACTOR 32" \
        "-nbodies 15000 -k 20 -FACTOR 16" \
        "-nbodies 10000 -FACTOR 40" \
        "-nbodies 20000 -FACTOR 16" \
        "-file ${GRAPH_PATH}/inp.txt -FACTOR 8" \
        "-file ${GRAPH_PATH}/kron_g500-logn21.egr -FACTOR 16" \
        "-file ${GRAPH_PATH}/rmat22.sym.egr -FACTOR 2" \
        )
# Total number of times each benchmark should run in one setting
ITERATIONS=1
# Total number of threads in each experiment across each build and across each benchmark
THREADS=( 32 )

export HCLIB_STATS=1
#export HCLIB_BIND_THREADS=1
#############################################
######### NO MODIFICATIONS BELOW ############
#############################################

launch() {
    mkdir Results
    config=$1
    echo "=============Launching experiment: $config==============="
    for thread in "${THREADS[@]}"; do
        current_iteration=0
	export HCLIB_WORKERS=$thread

	for exe in "${NAMES[@]}"; do
                FILE="./Results/${NAMES[$exe]}.threads-$thread.log"
                  chmod u+w $FILE
                  echo "" > $FILE
                

	done
        while [ $current_iteration -lt $ITERATIONS ]; do
            for exe in "${!BENCHMARKS[@]}"; do
                FILE="./Results/${NAMES[$exe]}.threads-$thread.log"
                    echo "Currently Running: $FILE "
                    ./${BENCHMARKS[$exe]} ${ARGS[$exe]} 2>&1 |  tee out
                    if [ `cat out | grep "TEST PASSED" | wc -l` -eq 0 ]; then
                        echo "ERROR: $FILE did not give Success. Not appending result..."
                    else
                        echo "Test Success: $FILE"
                        cat out >> $FILE
                    fi
            done
            current_iteration=`expr $current_iteration + 1`
        done
    done
}

launch