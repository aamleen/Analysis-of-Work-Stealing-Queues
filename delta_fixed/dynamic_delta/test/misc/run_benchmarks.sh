
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/dynamic_delta/hclib-install/bin/hclib_setup_env.sh
BENCHMARKS=( "fib" \
            "nqueens" \
            "matmul" \
            "heat" )

Args=( "48" \
        "14" \
        "50000" \
        "5" )

ITERATIONS=3

for (( i=0; i<${#BENCHMARKS[@]}; i++ )); do
    echo "Running ${BENCHMARKS[$i]} with ${Args[$i]} args"
    for (( j=0; j<$ITERATIONS; j++ )); do
        echo "Iteration $j"
        HCLIB_WORKERS=32 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${BENCHMARKS[$i]} ${Args[$i]} >> Results_DynDel/${BENCHMARKS[$i]}_${Args[$i]}.txt
    done
done
