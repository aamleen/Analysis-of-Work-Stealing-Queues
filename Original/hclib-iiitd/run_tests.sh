cd test/misc
./run_script.sh
clear
benchmarks=("fib" "heat")
param=(42 3)
counter=0
for j in ${!benchmarks[@]}
do
    test=${benchmarks[$j]}
    param_j=${param[$j]}
    echo "Running $test($param_j)"
    for i in {1..5}
    do
        > /mnt/hdd2/home/aamleen2022/FenceFree_Deq/results/hclib_${test}_${param_j}_${i}.txt
        HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${test} ${param_j} >> /mnt/hdd2/home/aamleen2022/FenceFree_Deq/results/hclib_${test}_${param_j}_${i}.txt
    done
done