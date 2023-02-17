cd ../../
./clean.sh
./install.sh
source /mnt/hdd2/home/aamleen2022/FenceFree_Deq/hclib-install/bin/hclib_setup_env.sh
cd test/misc
make

cd /mnt/hdd2/home/aamleen2022/FenceFree_Deq/test/Galois/BarnesHut
make

cd /mnt/hdd2/home/aamleen2022/FenceFree_Deq/test/misc