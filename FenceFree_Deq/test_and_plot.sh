# run a lopp from 20 to -1
delta=( $(seq -1 20 ) )
for i in {20..-1}
do
    ./run_FF_tests.sh $i
    sleep 5
done
./../Original/hclib-iiitd/run_tests.sh
benchmarks=("fib","heat", "matmul", "qsort")
param=(42,3,1024,100000000)
clear
echo "Plotting now"
./plot.sh