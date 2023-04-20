cd ../../
./clean.sh
./install.sh
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hclib/hclib-install/bin/hclib_setup_env.sh
cd test/misc
make


cd ../IMSuite_BM/timing/iterative/parallel
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hclib/hclib-install/bin/hclib_setup_env.sh
make clean
make

cd ../../recursive/parallel
source /home/anuj2022/Analysis-of-Work-Stealing-Queues-FenceFreeWs/delta_fixed/hclib/hclib-install/bin/hclib_setup_env.sh
make clean
make
