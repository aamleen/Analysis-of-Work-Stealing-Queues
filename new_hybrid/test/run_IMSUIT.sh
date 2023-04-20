iterative=('dijkstraRouting_parallel' 'bfsBellmanFord_parallel' 'byzantine_parallel' 'kcommitte_parallel' 'leader_elect_lcr_parallel' 'bfsDijkstra_parallel' 'leader_elect_dp_parallel')
iterative_file=('inputdijkstraRouting_600.txt' 'inputbfsBellmanFord_50000.txt' 'Byzantine_400.txt' 'inputKCommittee_32768.txt' 'inputleaderelectlcr_131072.txt' 'bfsDijkstra_10240.txt' 'inputleaderelectdp_2048.txt')


recusrive=('bfsDijkstra_parallel' 'byzantine_parallel' 'dijkstraRouting_parallel')
recursive_file=('bfsDijkstra_10240.txt' 'Byzantine_400.txt' 'inputdijkstraRouting_600.txt')

path='/home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/test/IMS_DVH_10ns/'

cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/test/IMSuite_BM/timing/iterative/parallel



# # use index and run command to run program at uterative with same file index input
# for i in "${!iterative[@]}"; do
#     # echo "Running ${iterative[$i]} with ${iterative_file[ $i ]}"
#     # run command
#     # run for 5 iterations
#     for j in {1..5}
#     do
#         sleep 5
#         echo "Running ${iterative[$i]} with ${iterative_file[ $i ]} for iteration $j"
#         source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/hclib/hclib-install/bin/hclib_setup_env.sh
#         HCLIB_WORKERS=32 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${iterative[$i]} -in /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/IMSuite_BM/IMSuite_Input/${iterative_file[ $i ]} -out demo.txt >> ${path}${iterative_file[$i]}_itr.txt
#     done
# done

# cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/hclib/test/IMSuite_BM/timing/recursive/parallel

# echo "Done with iterative"

# for i in "${!recusrive[@]}"; do
#     # echo "Running ${recusrive[$i]} with ${recursive_file[ $i ]}"
#     # run command
#     for j in {1..5}
#     do
#         sleep 5
#         echo "Running ${recusrive[$i]} with ${recusrive[ $i ]} for iteration $j"
#         source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/hclib/hclib-install/bin/hclib_setup_env.sh
#         HCLIB_WORKERS=32 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${recusrive[$i]} -in /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/IMSuite_BM/IMSuite_Input/${recursive_file[ $i ]} -out demo.txt >> ${path}${recursive_file[$i]}_rec.txt
#     done
# done





# use index and run command to run program at uterative with same file index input
for i in "${!iterative[@]}"; do
    # echo "Running ${iterative[$i]} with ${iterative_file[ $i ]}"
    # run command
    # run for 5 iterations
    for j in {1..3}
    do
        sleep 5
        echo "Running ${iterative[$i]} with ${iterative_file[ $i ]} for iteration $j"
        source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/hclib-install/bin/hclib_setup_env.sh
        HCLIB_WORKERS=32 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${iterative[$i]} -in /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/IMSuite_BM/IMSuite_Input/${iterative_file[ $i ]} -out demo.txt >> ${path}${iterative_file[$i]}_itr.txt
    done
done

cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/test/IMSuite_BM/timing/recursive/parallel

echo "Done with iterative"

for i in "${!recusrive[@]}"; do
    # echo "Running ${recusrive[$i]} with ${recursive_file[ $i ]}"
    # run command
    for j in {1..4}
    do
        sleep 5
        echo "Running ${recusrive[$i]} with ${recusrive[ $i ]} for iteration $j"
        source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/hclib-install/bin/hclib_setup_env.sh
        HCLIB_WORKERS=32 HCLIB_BIND_THREADS=true HCLIB_STATS=1 ./${recusrive[$i]} -in /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/IMSuite_BM/IMSuite_Input/${recursive_file[ $i ]} -out demo.txt >> ${path}${recursive_file[$i]}_rec.txt
    done
done