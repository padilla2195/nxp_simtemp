echo "Building Kernel Module"

# Go to Kernel folder
cd ..
cd kernel/
make all

echo "Building test app"

# Go to user folder
cd ..
cd user/cli
gcc main.c