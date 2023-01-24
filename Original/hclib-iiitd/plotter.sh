arg=40
delta="org"
cd test/misc
for i in {1..5}
do
    > /mnt/hdd2/home/aamleen2022/Original/hclib-iiitd/results/fib_org_${arg}_${delta}_${i}.txt
    HCLIB_WORKERS=20 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./fib ${arg} >> /mnt/hdd2/home/aamleen2022/Original/hclib-iiitd/results/fib_org_${arg}_${delta}_${i}.txt
done