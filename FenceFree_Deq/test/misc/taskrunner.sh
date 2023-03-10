source /mnt/hdd2/home/aamleen2022/FenceFree_Deq/hclib-install/bin/hclib_setup_env.sh

delta=2

# declare variables
param=40960
name="matmul"
# truncate txt files
for i in {1..6}
do
    sleep 2
    echo "Iteration $i"
    HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${name} $param > ./results/${name}_${param}_${i}_${delta}.txt
done

param=45
name="fib"
sleep 10
# truncate txt files
for i in {1..6}
do
    sleep 2
    echo "Iteration $i"
    HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${name} $param > ./results/${name}_${param}_${i}_${delta}.txt
done


param=14
name="nqueens"
sleep 10
# truncate txt files
for i in {1..6}
do
    sleep 2
    echo "Iteration $i"
    HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${name} $param > ./results/${name}_${param}_${i}_${delta}.txt
done