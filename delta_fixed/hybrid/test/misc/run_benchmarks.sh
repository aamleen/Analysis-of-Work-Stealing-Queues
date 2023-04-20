source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hybrid/hclib-install/bin/hclib_setup_env.sh
BENCHMARKS=( "nqueens" \
                "matmul" \
                "heat" )

Args=( "14" \
         "50000" \
         "5" )

ITERATIONS=5

for (( i=0; i<${#BENCHMARKS[@]}; i++ )); do
    echo "Running ${BENCHMARKS[$i]} with ${Args[$i]} args"
    for (( j=0; j<$ITERATIONS; j++ )); do
        echo "Iteration"$j
        #store the results in Results/BENCHMARKS[i]_ARGS[i].txt
        HCLIB_WORKERS=32 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${BENCHMARKS[$i]} ${Args[$i]} > Results_hybrid_1/${BENCHMARKS[$i]}_${Args[$i]}_$j.txt
    done
done