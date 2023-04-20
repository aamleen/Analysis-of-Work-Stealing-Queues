
cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/FF/test/misc
./run_script.sh
cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/FF/test
echo "Running FF"
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/FF/hclib-install/bin/hclib_setup_env.sh
./run_IMSUIT.sh

cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/test/misc
./run_script.sh
cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid/test
echo "Running FF"
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/new_hybrid//hclib-install/bin/hclib_setup_env.sh
./run_IMSUIT.sh

cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hybrid/test/misc
./run_script.sh
cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hybrid/test
echo "Running hclib"
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hclib/hclib-install/bin/hclib_setup_env.sh
./run_IMSUIT.sh

# sleep 10
# cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/FF/test/misc
# ./run_script.sh
# cd ../
# ./run_IMSUIT.sh
# sleep 10
# cd /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hybrid/test/misc
# ./run_script.sh
# cd ../
# ./run_IMSUIT.sh