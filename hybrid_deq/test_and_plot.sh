# run a lopp from 20 to -1
delta=( $(seq -7 4 ) )
for i in {4..-7}
do
    ./run_FF_tests.sh $i
    sleep 5
done
./../Original/hclib-iiitd/run_tests.sh
clear
echo "Plotting now"
./plot.sh