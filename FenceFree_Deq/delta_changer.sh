cd /mnt/hdd2/home/aamleen2022/FenceFree_Deq/src
delta=$1
delta2=$(($delta+1))
arg=45
echo "delta value is $delta"
# replace the delta value in the hclib-deq.cpp file
sed -i "s/delta $delta2/delta $delta/1" hclib-deque.c

cd ../test/misc
./run_script.sh
# truncate the file
for i in {1..5}
do
    > /mnt/hdd2/home/aamleen2022/FenceFree_Deq/results/fib_org_${arg}_${delta}_${i}.txt
    HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./fib ${arg} >> /mnt/hdd2/home/aamleen2022/FenceFree_Deq/results/fib_org_${arg}_${delta}_${i}.txt
done