echo "Building ROS nodes"

cd Examples/ROS/ORB_SLAM2
mkdir build
cd build
cmake .. -G"Eclipse CDT4 - Unix Makefiles" -DROS_BUILD_TYPE=Release
make -j
