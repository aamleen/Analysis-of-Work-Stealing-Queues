delta=( $(seq -7 4 ) )
benchmarks=("fib" "heat")
param=(42 3)
for j in ${!benchmarks[@]}
do
    python3 plot_graphs.py ${benchmarks[$j]} ${param[$j]} ${delta[@]}
done