
# Load the module
cd ..
cd kernel/
sudo insmod nxp_simtemp.ko

# Load the test application
cd ..
cd user/cli
sudo ./a.out

# Unload the module
sudo rmmod nxp_simtemp.ko
