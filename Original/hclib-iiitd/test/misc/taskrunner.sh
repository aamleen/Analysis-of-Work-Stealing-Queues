source /mnt/hdd2/home/aamleen2022/Original/hclib-iiitd/hclib-install/bin/hclib_setup_env.sh


# declare variables
# param=40960
# name="matmul"

# param=50
# name="fib"

param=14
name="nqueens"

# truncate txt files
for i in {1..6}
do
    sleep 2
    echo "Iteration $i"
    HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${name} $param > ./results/${name}_${param}_${i}.txt
done