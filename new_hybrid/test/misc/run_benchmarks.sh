source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/hclib-install/bin/hclib_setup_env.sh
BENCHMARKS=( "fib" \
            "nqueens" \
            "matmul" )

Args=( "48" \
        "14" \
        "41840" )

ITERATIONS=3

for (( i=0; i<${#BENCHMARKS[@]}; i++ )); do
    echo "Running ${BENCHMARKS[$i]} with ${Args[$i]} args"
    for (( j=0; j<$ITERATIONS; j++ )); do
        echo "Iteration $j"
        HCLIB_WORKERS=32 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${BENCHMARKS[$i]} ${Args[$i]} >> Results_20Apr_DVH_50ns/${BENCHMARKS[$i]}_${Args[$i]}.txt
    done
done