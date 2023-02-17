# truncate txt files


for i in {1..6}
do
    sleep 2

    echo "Iteration $i"
    HCLIB_STATS=1 HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true ./fib 42 > ./results/ff_same_both_$i.txt
    
    sleep 2

    cd /mnt/hdd2/home/aamleen2022/Original/hclib-iiitd/test/misc
    HCLIB_STATS=1 HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true ./fib 42 > /mnt/hdd2/home/aamleen2022/hybrid_deq/test/misc/results/org_same_both_$i.txt
    cd /mnt/hdd2/home/aamleen2022/hybrid_deq/test/misc
done