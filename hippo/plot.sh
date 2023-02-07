delta=( $(seq 0 21 ) )
benchmarks=("fib" "heat" "matmul" "qsort")
param=(42 3 1024 10000000)
for j in ${!benchmarks[@]}
do
    python3 plot_graphs.py ${benchmarks[$j]} ${param[$j]} ${delta[@]}
done