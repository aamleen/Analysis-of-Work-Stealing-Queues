cd /mnt/hdd2/home/aamleen2022/FenceFree_Deq/src
delta=$1
delta2=$(($delta+1))
echo "delta value is $delta"
# replace the delta value in the hclib-deq.cpp file
sed -i "s/delta $delta2/delta $delta/1" hclib-deque.c

cd ../test/misc
./run_script.sh
clear
# truncate the file
benchmarks=("fib" "heat" "matmul" "qsort")
param=(42 3 1024 100000000)
counter=0
for j in ${!benchmarks[@]}
do
    test=${benchmarks[$j]}
    param_j=${param[$j]}
    echo "Running $test($param_j) with delta ${delta}"
    for i in {1..5}
    do
        > /mnt/hdd2/home/aamleen2022/FenceFree_Deq/results/FF_${test}_${param_j}_${delta}_${i}.txt
        HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${test} ${param_j} >> /mnt/hdd2/home/aamleen2022/FenceFree_Deq/results/FF_${test}_${param_j}_${delta}_${i}.txt
    done
done