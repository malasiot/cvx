# cvx
General purpose library with various computer vision related modules

To build:

install dependencies:

OpenCV >3, Eigen, LAPACK, F2C (sudo apt-get install libeigen3-dev libblas-dev liblapack-dev libf2c-dev)

For viz: libglfw3-dev, libassimp-dev, libqtbase5-dev, libqtdeclarative5-dev

mkdir build; cd build; cmake .. ; make install ;

To use in your project:

find_package(cvx REQUIRED COMPONENTS util calibration ...)

...

target_link_libraries(my_target cvx_util cvx_calibration ...)

